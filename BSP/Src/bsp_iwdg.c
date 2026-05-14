/**
 * @file     bsp_iwdg.c
 * @author   Drinkto
 * @brief    IWDG driver
 * @version  V1.0
 * @date     2026-05-14
 */

/* Includes ------------------------------------------------------------------*/
#include "bsp_iwdg.h"
#include "stm32f1xx_ll_iwdg.h"
#include "stm32f1xx_ll_rcc.h"

#define BSP_IWDG_LSI_HZ        40000U       // LSI 名义频率
#define BSP_IWDG_RELOAD_MAX    0x0FFFU      // 12-bit 计数器
#define BSP_IWDG_READY_TIMEOUT 0x0000FFFFU  // 等待寄存器更新轮询次数

static uint32_t s_actual_timeout_ms = 0U;

/**
 * @brief  根据期望溢出时间，选择最合适的预分频和重载值
 * @param  _timeout_ms  期望溢出时间（毫秒）
 * @param  prescaler_ll 出参：选中的 LL 分频宏（LL_IWDG_PRESCALER_x）
 * @param  reload       出参：写入 IWDG_RLR 的 12-bit 重载值（1..4095）
 * @param  actual_ms    出参：根据所选分频/重载反算的实际名义超时（毫秒）
 * @retval BSP_IWDG_OK              成功
 * @retval BSP_IWDG_ERR_INVALID_ARG 入参为 0 或超出量程
 *
 * 选择策略：
 *   -
 * 候选分频按从小到大遍历（4/8/16/32/64/128/256），优先选最小分频，分辨率最高
 *   - 计算 reload = timeout_ms * LSI_HZ / (divider * 1000)
 *   - 第一个使 reload <= 0xFFF 的分频即为所选
 *
 * 量程：
 *   - 最小：divider=4,   reload=1     => ~0.1 ms
 *   - 最大：divider=256, reload=4095  => ~26214 ms
 */
static bsp_iwdg_status_t calc_prescaler_reload(uint32_t _timeout_ms,
                                               uint32_t *prescaler_ll,
                                               uint32_t *reload,
                                               uint32_t *actual_ms)
{
        if (_timeout_ms == 0U) {
                return BSP_IWDG_ERR_INVALID_ARG;
        }

        /* 候选分频表，按从小到大排列，分辨率优先 */
        static const struct {
                uint16_t divider;
                uint32_t ll_value;
        } presc_table[] = {
            {4U, LL_IWDG_PRESCALER_4},     {8U, LL_IWDG_PRESCALER_8},
            {16U, LL_IWDG_PRESCALER_16},   {32U, LL_IWDG_PRESCALER_32},
            {64U, LL_IWDG_PRESCALER_64},   {128U, LL_IWDG_PRESCALER_128},
            {256U, LL_IWDG_PRESCALER_256},
        };

        const uint32_t entries = sizeof(presc_table) / sizeof(presc_table[0]);

        for (uint32_t i = 0U; i < entries; ++i) {
                const uint32_t divider = (uint32_t)presc_table[i].divider;

                /* tick_us = divider * 1e6 / LSI_HZ
                 * reload  = timeout_ms * 1000 / tick_us
                 *         = timeout_ms * LSI_HZ / (divider * 1000)
                 * 乘积上限：26214 * 40000 ≈ 1.05e9 < 2^32，不会溢出 uint32_t */
                uint32_t reload_calc =
                    (_timeout_ms * BSP_IWDG_LSI_HZ) / (divider * 1000U);

                if (reload_calc == 0U) {
                        /* 当前分频下分辨率不足，继续尝试更大分频以获得有效计数
                         */
                        continue;
                }

                if (reload_calc <= BSP_IWDG_RELOAD_MAX) {
                        *prescaler_ll = presc_table[i].ll_value;
                        *reload = reload_calc;
                        *actual_ms =
                            (reload_calc * divider * 1000U) / BSP_IWDG_LSI_HZ;
                        return BSP_IWDG_OK;
                }
        }

        /* 即使最大分频 256 也无法满足 timeout_ms，超出 IWDG 量程 */
        return BSP_IWDG_ERR_INVALID_ARG;
}

/**
 * @brief 初始化并启动独立看门狗（启用后不可关闭，仅可被复位清除）
 * @param _timeout_ms 期望溢出时间（毫秒），有效范围约 1 ~ 26214
 *                   （基于 LSI≈40kHz、最大分频 256、RL 12bit）
 * @return BSP_IWDG_OK 成功；其他为错误码
 *
 * @note  喂狗周期必须严格小于 timeout_ms，建议留 30%~50% 裕量
 * @note  LSI 误差较大（约 ±10%），timeout_ms 是名义值，不是精确值
 */
bsp_iwdg_status_t bsp_iwdg_Init(uint32_t _timeout_ms)
{
        uint32_t prescaler_ll = 0U;
        uint32_t reload = 0U;
        uint32_t actual_ms = 0U;

        bsp_iwdg_status_t iwdg_status = calc_prescaler_reload(
            _timeout_ms, &prescaler_ll, &reload, &actual_ms);

        if (iwdg_status != BSP_IWDG_OK) {
                return iwdg_status;
        }

        /* 正确顺序：解锁 → 配置 → 等待 → 重载 → 启动 */
        LL_IWDG_EnableWriteAccess(IWDG);
        LL_IWDG_SetPrescaler(IWDG, prescaler_ll);
        LL_IWDG_SetReloadCounter(IWDG, reload);

        uint32_t guard = BSP_IWDG_READY_TIMEOUT;
        while (LL_IWDG_IsReady(IWDG) != 1U) {
                if (--guard == 0U) {
                        return BSP_IWDG_ERR_TIMEOUT;
                }
        }

        LL_IWDG_ReloadCounter(IWDG);
        LL_IWDG_Enable(IWDG);  // 最后启动
        s_actual_timeout_ms = actual_ms;
        return BSP_IWDG_OK;
}

/**
 * @brief 喂狗（重载计数器）
 * @note  必须由主循环 / 调度器周期性调用，禁止放在中断里
 */
void bsp_iwdg_Refresh(void) { LL_IWDG_ReloadCounter(IWDG); }

/**
 * @brief 查询本次启动是否由 IWDG 复位触发
 * @retval true  本次为 IWDG 看门狗复位
 * @retval false 其他原因（上电、外部 NRST、软复位等）
 * @note  应在 bsp_iwdg_Init 之前调用；调用后将清除 RCC CSR 复位标志
 */
bool bsp_iwdg_WasResetByWatchdog(void)
{
        bool flag = (LL_RCC_IsActiveFlag_IWDGRST() != 0U);
        LL_RCC_ClearResetFlags();
        return flag;
}

/**
 * @brief 获取实际配置的名义超时（毫秒）
 * @return 内部根据分频/RL 反算得到的名义溢出时间
 */
uint32_t bsp_iwdg_GetTimeoutMs(void) { return s_actual_timeout_ms; }
