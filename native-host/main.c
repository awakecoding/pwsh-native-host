
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <limits.h>
#endif

#include <nethost.h>

#include <coreclr_delegates.h>
#include <hostfxr.h>

static void* load_library(const char* path)
{
#ifdef _WIN32
    HMODULE hModule = LoadLibraryA(path);
    return (void*) hModule;
#else
    void* handle = dlopen(path, RTLD_LAZY | RTLD_LOCAL);
    return handle;
#endif
}

static void* get_proc_address(void* handle, const char* name)
{
#ifdef _WIN32
    HMODULE hModule = (HMODULE) handle;
    void* symbol = GetProcAddress(hModule, name);
    return symbol;
#else
    void* symbol = dlsym(handle, name);
    return symbol;
#endif
}

#ifndef _WIN32
extern void pthread_create();

void linker_dummy()
{
    // force linking pthread library
    pthread_create();
}
#endif

struct hostfxr_context
{
    hostfxr_initialize_for_dotnet_command_line_fn initialize_for_dotnet_command_line;
    hostfxr_initialize_for_runtime_config_fn initialize_for_runtime_config;
    hostfxr_get_runtime_property_value_fn get_runtime_property_value;
    hostfxr_set_runtime_property_value_fn set_runtime_property_value;
    hostfxr_get_runtime_properties_fn get_runtime_properties;
    hostfxr_run_app_fn run_app;
    hostfxr_get_runtime_delegate_fn get_runtime_delegate;
    hostfxr_close_fn close;

    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer;
    component_entry_point_fn entry_point;
};
typedef struct hostfxr_context HOSTFXR_CONTEXT;

bool load_hostfxr(HOSTFXR_CONTEXT* hostfxr)
{
    char hostfxr_path[1024];
    size_t path_size = sizeof(hostfxr_path) / sizeof(char);

    memset(hostfxr, sizeof(HOSTFXR_CONTEXT), 0);

    char* hostfxr_path_env = getenv("HOSTFXR_PATH");

    if (hostfxr_path_env)
    {
        // /usr/share/dotnet/host/fxr/5.0.0/libhostfxr.so
        strncpy(hostfxr_path, hostfxr_path_env, path_size);
    }
    #if 0
    else
    {
        int rc = get_hostfxr_path(hostfxr_path, &path_size, NULL);

        if (rc != 0)
            return false;
    }
    #endif

    void* lib_handle = load_library(hostfxr_path);

    if (!lib_handle) {
        printf("could not load %s\n", hostfxr_path);
    }

    hostfxr->initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)
        get_proc_address(lib_handle, "hostfxr_initialize_for_dotnet_command_line");
    hostfxr->initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)
        get_proc_address(lib_handle, "hostfxr_initialize_for_runtime_config");
    hostfxr->get_runtime_property_value = (hostfxr_get_runtime_property_value_fn)
        get_proc_address(lib_handle, "hostfxr_get_runtime_property_value");
    hostfxr->set_runtime_property_value = (hostfxr_set_runtime_property_value_fn)
        get_proc_address(lib_handle, "hostfxr_set_runtime_property_value");
    hostfxr->get_runtime_properties = (hostfxr_get_runtime_properties_fn)
        get_proc_address(lib_handle, "hostfxr_get_runtime_properties");
    hostfxr->run_app = (hostfxr_run_app_fn)
        get_proc_address(lib_handle, "hostfxr_run_app");
    hostfxr->get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)
        get_proc_address(lib_handle, "hostfxr_get_runtime_delegate");
    hostfxr->close = (hostfxr_close_fn)
        get_proc_address(lib_handle, "hostfxr_close");

    return true;
}

bool load_runtime(HOSTFXR_CONTEXT* hostfxr, const char* config_path)
{
    hostfxr_handle ctx = NULL;
    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = NULL;

    int rc = hostfxr->initialize_for_runtime_config(config_path, NULL, &ctx);

    if ((rc != 0) || (ctx == NULL)) {
        printf("initialize_for_runtime_config(%s) failure\n", config_path);
        return false;
    }

    rc = hostfxr->get_runtime_delegate(ctx,
        hdt_load_assembly_and_get_function_pointer,
        (void**) &load_assembly_and_get_function_pointer);

    if ((rc != 0) || (NULL == load_assembly_and_get_function_pointer)) {
        printf("get_runtime_delegate failure\n");
        return false;
    }

    hostfxr->close(ctx);

    hostfxr->load_assembly_and_get_function_pointer = load_assembly_and_get_function_pointer;

    return true;
}

bool load_assembly(HOSTFXR_CONTEXT* hostfxr, const char* assembly_path)
{
    component_entry_point_fn entry_point = NULL;
    const char* dotnet_type = "DotNetLib.Lib, DotNetLib";
    const char* dotnet_type_method = "Hello";

    int rc = hostfxr->load_assembly_and_get_function_pointer(
        assembly_path,
        dotnet_type,
        dotnet_type_method,
        NULL /* delegate_type_name */,
        NULL,
        (void**) &entry_point);

    if ((rc != 0) || (NULL == entry_point)) {
        printf("load_assembly_and_get_function_pointer failure!\n");   
    }

    hostfxr->entry_point = entry_point;

    return true;
}

struct entry_point_args
{
    const char* message;
    int number;
};
typedef struct entry_point_args ENTRY_POINT_ARGS;

bool call_entry_point(HOSTFXR_CONTEXT* hostfxr)
{
    ENTRY_POINT_ARGS args;

    for (int i = 0; i < 3; i++)
    {
        args.message = "from host!";
        args.number = i;
        hostfxr->entry_point(&args, sizeof(args));
    }
}

int main(int argc, char** argv)
{
    HOSTFXR_CONTEXT hostfxr;
    char runtime_config_path[1024];
    char assembly_path[1024];

    if (!load_hostfxr(&hostfxr)) {
        printf("failed to load hostfxr!\n");
        return -1;
    }

    if (getenv("HOSTFXR_RUNTIME_CONFIG")) {
        // full path to "DotNetLib.runtimeconfig.json"
        char* runtime_config_path_env = getenv("HOSTFXR_RUNTIME_CONFIG");
        strncpy(runtime_config_path, runtime_config_path_env, 1024);
    }

    if (getenv("HOSTFXR_ASSEMBLY_PATH")) {
        // full path to DotNetLib.dll
        char* assembly_path_env = getenv("HOSTFXR_ASSEMBLY_PATH");
        strncpy(assembly_path, assembly_path_env, 1024);
    }

    if (!load_runtime(&hostfxr, runtime_config_path)) {
        printf("failed to load runtime!\n");
        return -1;
    }

    if (!load_assembly(&hostfxr, assembly_path)) {
        printf("failed to load assembly!\n");
    }

    call_entry_point(&hostfxr);

    return 0;
}
