//===-- PlatformAndroid.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "lldb/Core/Module.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/ValueObject.h"
#include "lldb/Host/HostInfo.h"
#include "lldb/Utility/Log.h"
#include "lldb/Utility/Scalar.h"
#include "lldb/Utility/UriParser.h"

#include "AdbClient.h"
#include "PlatformAndroid.h"
#include "PlatformAndroidRemoteGDBServer.h"
#include "lldb/Target/Target.h"

using namespace lldb;
using namespace lldb_private;
using namespace lldb_private::platform_android;
using namespace std::chrono;

LLDB_PLUGIN_DEFINE(PlatformAndroid)

static uint32_t g_initialize_count = 0;
static const unsigned int g_android_default_cache_size =
    2048; // Fits inside 4k adb packet.

void PlatformAndroid::Initialize() {
  PlatformLinux::Initialize();

  if (g_initialize_count++ == 0) {
#if defined(__ANDROID__)
    PlatformSP default_platform_sp(new PlatformAndroid(true));
    default_platform_sp->SetSystemArchitecture(HostInfo::GetArchitecture());
    Platform::SetHostPlatform(default_platform_sp);
#endif
    PluginManager::RegisterPlugin(
        PlatformAndroid::GetPluginNameStatic(false),
        PlatformAndroid::GetPluginDescriptionStatic(false),
        PlatformAndroid::CreateInstance);
  }
}

void PlatformAndroid::Terminate() {
  if (g_initialize_count > 0) {
    if (--g_initialize_count == 0) {
      PluginManager::UnregisterPlugin(PlatformAndroid::CreateInstance);
    }
  }

  PlatformLinux::Terminate();
}

PlatformSP PlatformAndroid::CreateInstance(bool force, const ArchSpec *arch) {
  Log *log(GetLogIfAllCategoriesSet(LIBLLDB_LOG_PLATFORM));
  if (log) {
    const char *arch_name;
    if (arch && arch->GetArchitectureName())
      arch_name = arch->GetArchitectureName();
    else
      arch_name = "<null>";

    const char *triple_cstr =
        arch ? arch->GetTriple().getTriple().c_str() : "<null>";

    LLDB_LOGF(log, "PlatformAndroid::%s(force=%s, arch={%s,%s})", __FUNCTION__,
              force ? "true" : "false", arch_name, triple_cstr);
  }

  bool create = force;
  if (!create && arch && arch->IsValid()) {
    const llvm::Triple &triple = arch->GetTriple();
    switch (triple.getVendor()) {
    case llvm::Triple::PC:
      create = true;
      break;

#if defined(__ANDROID__)
    // Only accept "unknown" for the vendor if the host is android and if
    // "unknown" wasn't specified (it was just returned because it was NOT
    // specified).
    case llvm::Triple::VendorType::UnknownVendor:
      create = !arch->TripleVendorWasSpecified();
      break;
#endif
    default:
      break;
    }

    if (create) {
      switch (triple.getEnvironment()) {
      case llvm::Triple::Android:
        break;

#if defined(__ANDROID__)
      // Only accept "unknown" for the OS if the host is android and it
      // "unknown" wasn't specified (it was just returned because it was NOT
      // specified)
      case llvm::Triple::EnvironmentType::UnknownEnvironment:
        create = !arch->TripleEnvironmentWasSpecified();
        break;
#endif
      default:
        create = false;
        break;
      }
    }
  }

  if (create) {
    LLDB_LOGF(log, "PlatformAndroid::%s() creating remote-android platform",
              __FUNCTION__);
    return PlatformSP(new PlatformAndroid(false));
  }

  LLDB_LOGF(
      log, "PlatformAndroid::%s() aborting creation of remote-android platform",
      __FUNCTION__);

  return PlatformSP();
}

PlatformAndroid::PlatformAndroid(bool is_host)
    : PlatformLinux(is_host), m_sdk_version(0) {}

ConstString PlatformAndroid::GetPluginNameStatic(bool is_host) {
  if (is_host) {
    static ConstString g_host_name(Platform::GetHostPlatformName());
    return g_host_name;
  } else {
    static ConstString g_remote_name("remote-android");
    return g_remote_name;
  }
}

const char *PlatformAndroid::GetPluginDescriptionStatic(bool is_host) {
  if (is_host)
    return "Local Android user platform plug-in.";
  else
    return "Remote Android user platform plug-in.";
}

Status PlatformAndroid::ConnectRemote(Args &args) {
  m_device_id.clear();

  if (IsHost())
    return Status("can't connect to the host platform, always connected");

  if (!m_remote_platform_sp)
    m_remote_platform_sp = PlatformSP(new PlatformAndroidRemoteGDBServer());

  llvm::Optional<uint16_t> port;
  llvm::StringRef scheme, host, path;
  const char *url = args.GetArgumentAtIndex(0);
  if (!url)
    return Status("URL is null.");
  if (!UriParser::Parse(url, scheme, host, port, path))
    return Status("Invalid URL: %s", url);
  if (host != "localhost")
    m_device_id = std::string(host);

  auto error = PlatformLinux::ConnectRemote(args);
  if (error.Success()) {
    AdbClient adb;
    error = AdbClient::CreateByDeviceID(m_device_id, adb);
    if (error.Fail())
      return error;

    m_device_id = adb.GetDeviceID();
  }
  return error;
}

Status PlatformAndroid::GetFile(const FileSpec &source,
                                const FileSpec &destination) {
  if (IsHost() || !m_remote_platform_sp)
    return PlatformLinux::GetFile(source, destination);

  FileSpec source_spec(source.GetPath(false), FileSpec::Style::posix);
  if (source_spec.IsRelative())
    source_spec = GetRemoteWorkingDirectory().CopyByAppendingPathComponent(
        source_spec.GetCString(false));

  Status error;
  auto sync_service = GetSyncService(error);
  if (error.Fail())
    return error;

  uint32_t mode = 0, size = 0, mtime = 0;
  error = sync_service->Stat(source_spec, mode, size, mtime);
  if (error.Fail())
    return error;

  if (mode != 0)
    return sync_service->PullFile(source_spec, destination);

  auto source_file = source_spec.GetCString(false);

  Log *log(GetLogIfAllCategoriesSet(LIBLLDB_LOG_PLATFORM));
  LLDB_LOGF(log, "Got mode == 0 on '%s': try to get file via 'shell cat'",
            source_file);

  if (strchr(source_file, '\'') != nullptr)
    return Status("Doesn't support single-quotes in filenames");

  // mode == 0 can signify that adbd cannot access the file due security
  // constraints - try "cat ..." as a fallback.
  AdbClient adb(m_device_id);

  char cmd[PATH_MAX];
  snprintf(cmd, sizeof(cmd), "cat '%s'", source_file);

  return adb.ShellToFile(cmd, minutes(1), destination);
}

Status PlatformAndroid::PutFile(const FileSpec &source,
                                const FileSpec &destination, uint32_t uid,
                                uint32_t gid) {
  if (IsHost() || !m_remote_platform_sp)
    return PlatformLinux::PutFile(source, destination, uid, gid);

  FileSpec destination_spec(destination.GetPath(false), FileSpec::Style::posix);
  if (destination_spec.IsRelative())
    destination_spec = GetRemoteWorkingDirectory().CopyByAppendingPathComponent(
        destination_spec.GetCString(false));

  // TODO: Set correct uid and gid on remote file.
  Status error;
  auto sync_service = GetSyncService(error);
  if (error.Fail())
    return error;
  return sync_service->PushFile(source, destination_spec);
}

const char *PlatformAndroid::GetCacheHostname() { return m_device_id.c_str(); }

Status PlatformAndroid::DownloadModuleSlice(const FileSpec &src_file_spec,
                                            const uint64_t src_offset,
                                            const uint64_t src_size,
                                            const FileSpec &dst_file_spec) {
  if (src_offset != 0)
    return Status("Invalid offset - %" PRIu64, src_offset);

  return GetFile(src_file_spec, dst_file_spec);
}

Status PlatformAndroid::DisconnectRemote() {
  Status error = PlatformLinux::DisconnectRemote();
  if (error.Success()) {
    m_device_id.clear();
    m_sdk_version = 0;
  }
  return error;
}

uint32_t PlatformAndroid::GetDefaultMemoryCacheLineSize() {
  return g_android_default_cache_size;
}

uint32_t PlatformAndroid::GetSdkVersion() {
  if (!IsConnected())
    return 0;

  if (m_sdk_version != 0)
    return m_sdk_version;

  std::string version_string;
  AdbClient adb(m_device_id);
  Status error =
      adb.Shell("getprop ro.build.version.sdk", seconds(5), &version_string);
  version_string = llvm::StringRef(version_string).trim().str();

  if (error.Fail() || version_string.empty()) {
    Log *log = GetLogIfAllCategoriesSet(LIBLLDB_LOG_PLATFORM);
    LLDB_LOGF(log, "Get SDK version failed. (error: %s, output: %s)",
              error.AsCString(), version_string.c_str());
    return 0;
  }

  // FIXME: improve error handling
  llvm::to_integer(version_string, m_sdk_version);
  return m_sdk_version;
}

Status PlatformAndroid::DownloadSymbolFile(const lldb::ModuleSP &module_sp,
                                           const FileSpec &dst_file_spec) {
  // For oat file we can try to fetch additional debug info from the device
  ConstString extension = module_sp->GetFileSpec().GetFileNameExtension();
  if (extension != ".oat" && extension != ".odex")
    return Status(
        "Symbol file downloading only supported for oat and odex files");

  // If we have no information about the platform file we can't execute oatdump
  if (!module_sp->GetPlatformFileSpec())
    return Status("No platform file specified");

  // Symbolizer isn't available before SDK version 23
  if (GetSdkVersion() < 23)
    return Status("Symbol file generation only supported on SDK 23+");

  // If we already have symtab then we don't have to try and generate one
  if (module_sp->GetSectionList()->FindSectionByName(ConstString(".symtab")) !=
      nullptr)
    return Status("Symtab already available in the module");

  AdbClient adb(m_device_id);
  std::string tmpdir;
  Status error = adb.Shell("mktemp --directory --tmpdir /data/local/tmp",
                           seconds(5), &tmpdir);
  if (error.Fail() || tmpdir.empty())
    return Status("Failed to generate temporary directory on the device (%s)",
                  error.AsCString());
  tmpdir = llvm::StringRef(tmpdir).trim().str();

  // Create file remover for the temporary directory created on the device
  std::unique_ptr<std::string, std::function<void(std::string *)>>
  tmpdir_remover(&tmpdir, [&adb](std::string *s) {
    StreamString command;
    command.Printf("rm -rf %s", s->c_str());
    Status error = adb.Shell(command.GetData(), seconds(5), nullptr);

    Log *log(GetLogIfAllCategoriesSet(LIBLLDB_LOG_PLATFORM));
    if (log && error.Fail())
      LLDB_LOGF(log, "Failed to remove temp directory: %s", error.AsCString());
  });

  FileSpec symfile_platform_filespec(tmpdir);
  symfile_platform_filespec.AppendPathComponent("symbolized.oat");

  // Execute oatdump on the remote device to generate a file with symtab
  StreamString command;
  command.Printf("oatdump --symbolize=%s --output=%s",
                 module_sp->GetPlatformFileSpec().GetCString(false),
                 symfile_platform_filespec.GetCString(false));
  error = adb.Shell(command.GetData(), minutes(1), nullptr);
  if (error.Fail())
    return Status("Oatdump failed: %s", error.AsCString());

  // Download the symbolfile from the remote device
  return GetFile(symfile_platform_filespec, dst_file_spec);
}

bool PlatformAndroid::GetRemoteOSVersion() {
  m_os_version = llvm::VersionTuple(GetSdkVersion());
  return !m_os_version.empty();
}

llvm::StringRef
PlatformAndroid::GetLibdlFunctionDeclarations(lldb_private::Process *process) {
  SymbolContextList matching_symbols;
  std::vector<const char *> dl_open_names = { "__dl_dlopen", "dlopen" };
  const char *dl_open_name = nullptr;
  Target &target = process->GetTarget();
  for (auto name: dl_open_names) {
    target.GetImages().FindFunctionSymbols(
        ConstString(name), eFunctionNameTypeFull, matching_symbols);
    if (matching_symbols.GetSize()) {
       dl_open_name = name;
       break;
    }
  }
  // Older platform versions have the dl function symbols mangled
  if (dl_open_name == dl_open_names[0])
    return R"(
              extern "C" void* dlopen(const char*, int) asm("__dl_dlopen");
              extern "C" void* dlsym(void*, const char*) asm("__dl_dlsym");
              extern "C" int   dlclose(void*) asm("__dl_dlclose");
              extern "C" char* dlerror(void) asm("__dl_dlerror");
             )";

  return PlatformPOSIX::GetLibdlFunctionDeclarations(process);
}

AdbClient::SyncService *PlatformAndroid::GetSyncService(Status &error) {
  if (m_adb_sync_svc && m_adb_sync_svc->IsConnected())
    return m_adb_sync_svc.get();

  AdbClient adb(m_device_id);
  m_adb_sync_svc = adb.GetSyncService(error);
  return (error.Success()) ? m_adb_sync_svc.get() : nullptr;
}
