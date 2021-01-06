
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

#define HOSTFXR_MAX_PATH    1024

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
    hostfxr_handle context_handle;
};
typedef struct hostfxr_context HOSTFXR_CONTEXT;

bool load_hostfxr(HOSTFXR_CONTEXT* hostfxr, const char* hostfxr_path)
{
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

bool load_assembly(HOSTFXR_CONTEXT* hostfxr, const char* assembly_path, const char* type_name, const char* method_name)
{
    component_entry_point_fn entry_point = NULL;

    int rc = hostfxr->load_assembly_and_get_function_pointer(
        assembly_path,
        type_name,
        method_name,
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

bool run_sample()
{
    HOSTFXR_CONTEXT hostfxr;
    char base_path[HOSTFXR_MAX_PATH];
    char runtime_config_path[HOSTFXR_MAX_PATH];
    char assembly_path[HOSTFXR_MAX_PATH];

    const char* hostfxr_path = "/usr/share/dotnet/host/fxr/5.0.0/libhostfxr.so";

    if (!load_hostfxr(&hostfxr, hostfxr_path)) {
        printf("failed to load hostfxr!\n");
        return -1;
    }

    strncpy(base_path, "/opt/wayk/dev/pwsh-native-host/DotNetLib/bin/Release/net5.0/linux-x64", HOSTFXR_MAX_PATH);
    snprintf(runtime_config_path, HOSTFXR_MAX_PATH, "%s/%s.runtimeconfig.json", base_path, "DotNetLib");
    snprintf(assembly_path, HOSTFXR_MAX_PATH, "%s/%s.dll", base_path, "DotNetLib");

    if (!load_runtime(&hostfxr, runtime_config_path)) {
        printf("failed to load runtime!\n");
        return -1;
    }

    const char* type_name = "DotNetLib.Lib, DotNetLib";
    const char* method_name = "Hello";

    if (!load_assembly(&hostfxr, assembly_path, type_name, method_name)) {
        printf("failed to load assembly!\n");
    }

    call_entry_point(&hostfxr);
}

bool load_command(HOSTFXR_CONTEXT* hostfxr, int argc, const char** argv)
{
    hostfxr_handle ctx = NULL;
    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = NULL;

    int rc = hostfxr->initialize_for_dotnet_command_line(argc, argv, NULL, &ctx);

    if ((rc != 0) || (ctx == NULL)) {
        printf("hostfxr->initialize_for_dotnet_command_line() failure: 0x%08X\n", rc);
        return false;
    }

    rc = hostfxr->get_runtime_delegate(ctx,
        hdt_load_assembly_and_get_function_pointer,
        (void**) &load_assembly_and_get_function_pointer);

    if ((rc != 0) || (NULL == load_assembly_and_get_function_pointer)) {
        printf("get_runtime_delegate failure\n");
        return false;
    }

    hostfxr->context_handle = ctx;
    //hostfxr->close(ctx);

    hostfxr->load_assembly_and_get_function_pointer = load_assembly_and_get_function_pointer;

    return true;
}

bool run_pwsh()
{
    HOSTFXR_CONTEXT hostfxr;
    char base_path[HOSTFXR_MAX_PATH];
    char hostfxr_path[HOSTFXR_MAX_PATH];
    char runtime_config_path[HOSTFXR_MAX_PATH];
    char assembly_path[HOSTFXR_MAX_PATH];
    char command_path[HOSTFXR_MAX_PATH];

    strncpy(base_path, "/home/wayk/powershell-7.1.0", HOSTFXR_MAX_PATH);
    snprintf(hostfxr_path, HOSTFXR_MAX_PATH, "%s/libhostfxr.so", base_path);

    if (!load_hostfxr(&hostfxr, hostfxr_path)) {
        printf("failed to load hostfxr!\n");
        return false;
    }

    snprintf(runtime_config_path, HOSTFXR_MAX_PATH, "%s/%s.runtimeconfig.json", base_path, "pwsh");
    snprintf(assembly_path, HOSTFXR_MAX_PATH, "%s/%s.dll", base_path, "pwsh");
    snprintf(command_path, HOSTFXR_MAX_PATH, "%s/%s.dll", base_path, "pwsh");

    char* command_args[] = {
        command_path,
        "-NoLogo",
        "-NoExit",
        "-Command",
        "Write-Host 'Hello PowerShell Host'"
    };
    int command_argc = sizeof(command_args) / sizeof(char*);

    if (!load_command(&hostfxr, command_argc, (const char**) command_args)) {
        printf("failed to load runtime!\n");
        return false;
    }

    hostfxr.run_app(hostfxr.context_handle);

#if 0
    const char* type_name = "Microsoft.PowerShell.ManagedPSEntry";
    const char* method_name = "Main";

    if (!load_assembly(&hostfxr, assembly_path, type_name, method_name)) {
        printf("failed to load assembly!\n");
        return false;
    }
#endif

    return true;
}

int main(int argc, char** argv)
{
    //run_sample();
    run_pwsh();

    return 0;
}
