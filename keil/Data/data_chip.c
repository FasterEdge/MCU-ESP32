// data_chip.c — ChipData 实现（Keil/裸机 C 版，MCU 专有）
// MCU 专有 Data：芯片信息（平台差异由 fe_port 提供）。
//   info   返回型号 / 芯片 ID / 内核数 / 频率 / 闪存
#include "fe_data.h"
#include "fe_port.h"

fe_output_t data_chip_dispatch(void *inst, const char *act, const char *args) {
    (void)inst; (void)args;
    if (strcmp(act, "info") == 0) {
        char out[256];
        fe_port_chip_info(out, sizeof(out));
        return fe_ok(act, out);
    }
    return fe_err(act, "unsupported command");
}