/* bootstrap.c -- h-encore bootstrap menu
 *
 * Copyright (C) 2019 TheFloW
 * Mod 2021 Codiak
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */

#include <vitasdk.h>
#include "pspdebug.h"

#define RED       0xFF0000FF
#define MAGENTA   0xFFFF00FF

#define printf psvDebugScreenPrintf

#define VITASHELL_BASE_ADDRESS "https://raw.githubusercontent.com/TheOfficialFloW/VitaShell/master/release"

#define INCLUDE_EXTERN_RESOURCE(name) extern unsigned char _binary_res_##name##_start; extern unsigned char _binary_res_##name##_size; \

INCLUDE_EXTERN_RESOURCE(taihen_skprx);
INCLUDE_EXTERN_RESOURCE(henkaku_skprx);
INCLUDE_EXTERN_RESOURCE(henkaku_suprx);

const char ur0_tai_path[] = "ur0:tai";
const char taihen_skprx_path[] = "ur0:tai/taihen.skprx";
const char henkaku_skprx_path[] = "ur0:tai/henkaku.skprx";
const char henkaku_suprx_path[] = "ur0:tai/henkaku.suprx";

char *continue_config_name;
char *switch_config_msg;

const char config_path[] = "ur0:tai/config.txt";
const char portable_config_path[] = "ur0:tai/config_port.txt";
const char dock_config_path[] = "ur0:tai/config_dock.txt";
const char backup_config_path[] = "ur0:tai/config_bkup.txt";

const char recovery_config[] =
  "# For users plugins, you must refresh taiHEN from HENkaku Settings for\n"
  "# changes to take place.\n"
  "# For kernel plugins, you must reboot for changes to take place.\n"
  "*KERNEL\n"
  "ur0:tai/storagemgr.skprx\n"
  "# henkaku.skprx is hard-coded to load and is not listed here\n"
  "*main\n"
  "# main is a special titleid for SceShell\n"
  "ur0:tai/henkaku.suprx\n"
  "*NPXS10015\n"
  "# this is for modifying the version string\n"
  "ur0:tai/henkaku.suprx\n"
  "*NPXS10016\n"
  "# this is for modifying the version string in settings widget\n"
  "ur0:tai/henkaku.suprx\n";

enum Items {
  CONTINUE,
  SWITCH_MODE,
  RECOVERY_MODE,
  HENKAKU_CFW,
  VITASHELL_APP,
  NOTROPHY_MSG
};

const char *items[] = {
  "> Continue",
  "> Switch Mode",
  "> Recovery Mode",
  ">> HENkaku CFW",
  ">> VitaShell App",
  ">> No Trophy Message"
};

#define N_ITEMS (sizeof(items) / sizeof(char *))

int __attribute__((naked, noinline)) call_syscall(int a1, int a2, int a3, int num) {
  __asm__ (
    "mov r12, %0 \n"
    "svc 0 \n"
    "bx lr \n"
    : : "r" (num)
  );
}

int write_file(const char *file, const void *buf, int size) {
  SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0)
    return fd;

  int written = sceIoWrite(fd, buf, size);

  sceIoClose(fd);
  return written;
}

int exists(const char *path) {
  SceIoStat stat;

  if (sceIoGetstat(path, &stat) < 0)
    return 0;
  else
    return 1;
}

int load_sce_paf() {
  static uint32_t argp[] = { 0x400000, 0xEA60, 0x40000, 0, 0 };

  int result = -1;

  uint32_t buf[4];
  buf[0] = sizeof(buf);
  buf[1] = (uint32_t)&result;
  buf[2] = -1;
  buf[3] = -1;

  return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, buf);
}

int unload_sce_paf() {
  uint32_t buf = 0;
  return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &buf);
}

int promote_app(const char *path) {
  int res;

  load_sce_paf();

  res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = scePromoterUtilityInit();
  if (res < 0)
    return res;

  res = scePromoterUtilityPromotePkgWithRif(path, 1);
  if (res < 0)
    return res;

  res = scePromoterUtilityExit();
  if (res < 0)
    return res;

  res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  unload_sce_paf();

  return res;
}

int remove(const char *path) {
  SceUID dfd = sceIoDopen(path);
  if (dfd >= 0) {
    int res = 0;

    do {
      SceIoDirent dir;
      sceClibMemset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dfd, &dir);
      if (res > 0) {
        char new_path[256];
        sceClibSnprintf(new_path, sizeof(new_path), "%s/%s", path, dir.d_name);
        res = remove(new_path);
      }
    } while (res > 0);

    sceIoDclose(dfd);

    return sceIoRmdir(path);
  }

  return sceIoRemove(path);
}

int download(const char *src, const char *dst) {
  int ret;
  int statusCode;
  int tmplId = -1, connId = -1, reqId = -1;
  SceUID fd = -1;

  ret = sceHttpCreateTemplate("h-encore/1.00 libhttp/1.1", SCE_HTTP_VERSION_1_1, SCE_TRUE);
  if (ret < 0)
    goto ERROR_EXIT;

  tmplId = ret;

  ret = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
  if (ret < 0)
    goto ERROR_EXIT;

  connId = ret;

  ret = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
  if (ret < 0)
    goto ERROR_EXIT;

  reqId = ret;

  ret = sceHttpSendRequest(reqId, NULL, 0);
  if (ret < 0)
    goto ERROR_EXIT;

  ret = sceHttpGetStatusCode(reqId, &statusCode);
  if (ret < 0)
    goto ERROR_EXIT;

  if (statusCode == 200) {
    uint8_t buf[4096];
    uint64_t size = 0;
    uint32_t value = 0;

    ret = sceHttpGetResponseContentLength(reqId, &size);
    if (ret < 0)
      goto ERROR_EXIT;

    ret = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 6);
    if (ret < 0)
      goto ERROR_EXIT;

    fd = ret;

    int x = psvDebugScreenGetX();
    int y = psvDebugScreenGetY();

    while (1) {
      int read = sceHttpReadData(reqId, buf, sizeof(buf));

      if (read < 0) {
        ret = read;
        break;
      }

      if (read == 0)
        break;

      int written = sceIoWrite(fd, buf, read);

      if (written < 0) {
        ret = written;
        break;
      }

      value += read;

      printf("%d%%", (int)((value * 100) / (uint32_t)size));
      psvDebugScreenSetXY(x, y);
    }
  }

ERROR_EXIT:
  if (fd >= 0)
    sceIoClose(fd);

  if (reqId >= 0)
    sceHttpDeleteRequest(reqId);

  if (connId >= 0)
    sceHttpDeleteConnection(connId);

  if (tmplId >= 0)
    sceHttpDeleteTemplate(tmplId);

  return ret;
}

void init_net() {
  static char memory[16 * 1024];

  sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
  sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);

  SceNetInitParam param;
  param.memory = memory;
  param.size = sizeof(memory);
  param.flags = 0;

  sceNetInit(&param);
  sceNetCtlInit();

  sceSslInit(300 * 1024);
  sceHttpInit(40 * 1024);

  sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);
}

void finish_net() {
  sceSslTerm();
  sceHttpTerm();
  sceNetCtlTerm();
  sceNetTerm();
  sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);
  sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
}

typedef struct {
  char *src;
  char *dst;
} DownloadSrcDst;

DownloadSrcDst vitashell_src_dst[] = {
  { "eboot.bin",    "ux0:temp/app/eboot.bin" },
  { "icon0.png",    "ux0:temp/app/sce_sys/icon0.png" },
  { "param.sfo",    "ux0:temp/app/sce_sys/param.sfo" },
  { "bg.png",       "ux0:temp/app/sce_sys/livearea/contents/bg.png" },
  { "startup.png",  "ux0:temp/app/sce_sys/livearea/contents/startup.png" },
  { "template.xml", "ux0:temp/app/sce_sys/livearea/contents/template.xml" },
  { "head.bin",     "ux0:temp/app/sce_sys/package/head.bin" },
};

int download_vitashell() {
  char url[256];
  int res;
  int i;
  
  printf(" > Installing VitaShell...\n");
  init_net();

  remove("ux0:patch/VITASHELL");

  sceIoMkdir("ux0:VitaShell", 0777);
  sceIoMkdir("ux0:temp", 6);
  sceIoMkdir("ux0:temp/app", 6);
  sceIoMkdir("ux0:temp/app/sce_sys", 6);
  sceIoMkdir("ux0:temp/app/sce_sys/livearea", 6);
  sceIoMkdir("ux0:temp/app/sce_sys/livearea/contents", 6);
  sceIoMkdir("ux0:temp/app/sce_sys/package", 6);

  for (i = 0; i < sizeof(vitashell_src_dst) / sizeof(DownloadSrcDst); i++) {
    printf(" > Downloading %s...", vitashell_src_dst[i].src);
    sceClibSnprintf(url, sizeof(url), "%s/%s", VITASHELL_BASE_ADDRESS, vitashell_src_dst[i].src);
    res = download(url, vitashell_src_dst[i].dst);
    printf("\n");
    if (res < 0)
      return res;
  }

  printf(" > Installing application...\n");
  res = promote_app("ux0:temp/app");
  if (res < 0)
    return res;

  finish_net();

  return 0;
}

int install_henkaku() {
  int res;

  printf(" > Installing HENkaku...\n");
  sceIoMkdir(ur0_tai_path, 6);

  res = write_file(taihen_skprx_path, (void *)&_binary_res_taihen_skprx_start, (int)&_binary_res_taihen_skprx_size);
  if (res < 0)
    return res;
  res = write_file(henkaku_skprx_path, (void *)&_binary_res_henkaku_skprx_start, (int)&_binary_res_henkaku_skprx_size);
  if (res < 0)
    return res;
  res = write_file(henkaku_suprx_path, (void *)&_binary_res_henkaku_suprx_start, (int)&_binary_res_henkaku_suprx_size);
  if (res < 0)
    return res;

  return 0;
}

int write_config(const int switch_config) {
  int fd;
  sceIoMkdir(ur0_tai_path, 6);
  
  if (switch_config == 0) {
    printf(" > Backing up Last Config...\n");
    printf(" > Loading Recovery Config...\n");
    
    fd = sceIoRemove(backup_config_path);
    fd = sceIoOpen(config_path, SCE_O_RDONLY, 6);
    fd = sceIoRename(config_path, backup_config_path);
    sceIoClose(fd);
    
    fd = sceIoOpen(config_path, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
    sceIoWrite(fd, recovery_config, sizeof(recovery_config) - 1);
    sceIoClose(fd);
  }
  else if (exists(portable_config_path)) {
    printf(" > Switching to Portable Mode...\n");
    
    fd = sceIoRemove(dock_config_path);
    fd = sceIoOpen(config_path, SCE_O_RDONLY, 6);
    fd = sceIoRename(config_path, dock_config_path);
    sceIoClose(fd);
    
    fd = sceIoOpen(portable_config_path, SCE_O_RDONLY, 6);
    fd = sceIoRename(portable_config_path, config_path);
    sceIoClose(fd);
  }
  else if (exists(dock_config_path)) {
    printf(" > Switching to Dock Mode...\n");
    
    fd = sceIoRemove(portable_config_path);
    fd = sceIoOpen(config_path, SCE_O_RDONLY, 6);
    fd = sceIoRename(config_path, portable_config_path);
    sceIoClose(fd);
    
    fd = sceIoOpen(dock_config_path, SCE_O_RDONLY, 6);
    fd = sceIoRename(dock_config_path, config_path);
    sceIoClose(fd);
  }
  else {
    printf(" > Creating Dock Config...\n");
    printf(" > Loading Last Config as Portable Config...\n");
    
    fd = sceIoOpen(dock_config_path, SCE_O_TRUNC | SCE_O_CREAT | SCE_O_WRONLY, 6);
    sceIoWrite(fd, recovery_config, sizeof(recovery_config) - 1);
    sceIoClose(fd);
  }
  
  return 0;
}

int personalize_savedata(int syscall_id) {
  int res;
  int fd;
  uint64_t aid;
  
  printf(" > Personalising savedata...\n");
  res = call_syscall(sceKernelGetProcessId(), 0, 0, syscall_id + 4);
  if (res < 0 && res != 0x80800003)
    return res;

  res = sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &aid, sizeof(uint64_t));
  if (res < 0)
    return res;

  fd = sceIoOpen("savedata0:sce_sys/param.sfo", SCE_O_RDWR, 0777);
  if (fd < 0)
    return fd;

  sceIoLseek(fd, 0xe4, SCE_SEEK_SET);
  sceIoWrite(fd, &aid, sizeof(uint64_t));
  sceIoClose(fd);

  return 0;
}

int enter_cross = 0;
uint32_t old_buttons = 0, current_buttons = 0, pressed_buttons = 0;

void read_pad(void) {
  SceCtrlData pad;
  sceCtrlPeekBufferPositive(0, &pad, 1);

  old_buttons = current_buttons;
  current_buttons = pad.buttons;
  pressed_buttons = current_buttons & ~old_buttons;
}

int wait_confirm(const char *msg) {
  printf(msg);
  printf(" > Press %c to confirm or %c to decline.\n", enter_cross ? 'X' : 'O', enter_cross ? 'O' : 'X');

  while (1) {
    read_pad();

    if ((enter_cross && pressed_buttons & SCE_CTRL_CROSS) ||
        (!enter_cross && pressed_buttons & SCE_CTRL_CIRCLE)) {
      return 1;
    }

    if ((enter_cross && pressed_buttons & SCE_CTRL_CIRCLE) ||
        (!enter_cross && pressed_buttons & SCE_CTRL_CROSS)) {
      return 0;
    }

    sceKernelDelayThread(10 * 1000);
  }

  return 0;
}

void print_result(int res) {
  if (res < 0)
    printf(" > Failed! 0x%08X\n", res);
  else
    printf(" > Success!\n");
  sceKernelDelayThread((res < 0) ? (5 * 1000 * 1000) : (1 * 1000 * 1000));
}

int print_menu(int sel) {
  int i;

  psvDebugScreenSetXY(0, 0);
  psvDebugScreenSetTextColor(RED);
  printf("\n Mod of h-encore-2 by Codiak\n");
  printf(" ---------------------------\n\n");
  
  psvDebugScreenSetTextColor(MAGENTA);
  printf(continue_config_name);
  psvDebugScreenSetTextColor(RED);
  printf(" > Exit\n");
  printf(" >> Refresh\n\n");

  for (i = 0; i < N_ITEMS; i++) {
    psvDebugScreenSetTextColor(sel == i ? MAGENTA : RED);
    printf(" [%c] %s\n", sel == i ? 'X' : ' ', items[i]);
  }

  psvDebugScreenSetTextColor(RED);
  printf("\n ---------------------------\n\n");

  return 0;
}

void _start() __attribute__ ((weak, alias ("module_start")));
int module_start(SceSize args, void *argp) {
  SceAppUtilInitParam init_param;
  SceAppUtilBootParam boot_param;
  int syscall_id;
  int enter_button;
  int sel;
  int res;

  syscall_id = *(uint16_t *)argp;

  sceAppMgrDestroyOtherApp();

  sceShellUtilInitEvents(0);
  sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

  sceClibMemset(&init_param, 0, sizeof(SceAppUtilInitParam));
  sceClibMemset(&boot_param, 0, sizeof(SceAppUtilBootParam));
  sceAppUtilInit(&init_param, &boot_param);

  sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
  enter_cross = enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CROSS;

  psvDebugScreenInit();
  psvDebugScreenClearLineDisable();
  
  if (exists(portable_config_path)) {
    continue_config_name = " [DOCK MODE]\n";
    switch_config_msg = " > Switch to Portable Config?\n";
  }
  else if (exists(dock_config_path)) {
    continue_config_name = " [PORTABLE MODE]\n";
    switch_config_msg = " > Switch to Dock Config?\n";
  }
  else {
    continue_config_name = " [NO MODE]\n";
    switch_config_msg = " > No mode defined. Load Last Config as Portable Config and create Dock Config to fill in?\n";
  }

  sel = 0;
  print_menu(sel);

  while (1) {
    read_pad();

    if (pressed_buttons & SCE_CTRL_UP) {
      if (sel > 0)
        sel--;
      else
        sel = N_ITEMS - 1;

      print_menu(sel);
    }

    if (pressed_buttons & SCE_CTRL_DOWN) {
      if (sel < N_ITEMS - 1)
        sel++;
      else
        sel = 0;

      print_menu(sel);
    }

    if ((enter_cross && pressed_buttons & SCE_CTRL_CROSS) ||
        (!enter_cross && pressed_buttons & SCE_CTRL_CIRCLE)) {
      psvDebugScreenSetTextColor(RED);

      if (sel == CONTINUE) {
        printf(" > Loading Last Config...\n");
        sceKernelDelayThread(500 * 1000);
        break;
      }
      else if (sel == SWITCH_MODE) {
        if (wait_confirm(switch_config_msg)) {
          sceKernelDelayThread(500 * 1000);
          res = write_config(1);
          break;
        } else {
          sel = 0;
          psvDebugScreenClear();
          print_menu(sel);
          continue;
        }
      }
      else if (sel == RECOVERY_MODE) {
        if (wait_confirm(" > Backup Last Config and Reset to Base Config with SD2VITA?\n")) {
          sceKernelDelayThread(500 * 1000);
          res = write_config(0);
          break;
        } else {
          sel = 0;
          psvDebugScreenClear();
          print_menu(sel);
          continue;
        }
      }
      else if (sel == HENKAKU_CFW) {
        if (wait_confirm(" > Install the CFW framework?\n")) {
          sceKernelDelayThread(500 * 1000);
          res = install_henkaku();
        } else {
          sel = 0;
          psvDebugScreenClear();
          print_menu(sel);
          continue;
        }
      }
      else if (sel == VITASHELL_APP) {
        if (wait_confirm(" > Install the multi-functional file manager?\n")) {
          sceKernelDelayThread(500 * 1000);
          res = download_vitashell();
        } else {
          sel = 0;
          psvDebugScreenClear();
          print_menu(sel);
          continue;
        }
      }
      else if (sel == NOTROPHY_MSG) {
        if (wait_confirm(" > Personalise savedata to remove trophy message?\n")) {
          sceKernelDelayThread(500 * 1000);
          res = personalize_savedata(syscall_id);
        } else {
          sel = 0;
          psvDebugScreenClear();
          print_menu(sel);
          continue;
        }
      }

      print_result(res);

      sel = 0;
      psvDebugScreenClear();
      print_menu(sel);
    }

    sceKernelDelayThread(10 * 1000);
  }

  sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);

  // Install HENkaku if any of the modules are missing
  if (!exists(henkaku_suprx_path) ||
      !exists(henkaku_skprx_path) ||
      !exists(taihen_skprx_path)) {
    sceKernelDelayThread(500 * 1000);
    res = install_henkaku();
    print_result(res);
  }

  // Write taiHEN configs if config.txt doesn't exist
  if (!exists(config_path)) {
    sceKernelDelayThread(500 * 1000);
    res = write_config(0);
    print_result(res);
  }

  // Remove pkg patches
  res = call_syscall(0, 0, 0, syscall_id + 1);

  if (res >= 0) {
    // Start HENkaku
    res = call_syscall(0, 0, 0, syscall_id + 0);
  } else {
    // Remove sig patches
    call_syscall(0, 0, 0, syscall_id + 2);
  }

  // Clean up
  call_syscall(0, 0, 0, syscall_id + 3);

  if (res < 0 && res != 0x8002D013 && res != 0x8002D017) {
    printf(" > Failed to load HENkaku! 0x%08X\n", res);
    printf(" > Please relaunch exploit and select 'HENkaku CFW'.\n");
    sceKernelDelayThread(5 * 1000 * 1000);
  }

  sceKernelExitProcess(0);
  return 0;
}
