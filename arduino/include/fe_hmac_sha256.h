// fe_hmac_sha256.h — HMAC-SHA256 纯 C 实现（零依赖）
// 供 Arduino / Keil / 裸机版本共用，避免依赖平台 mbedTLS。
#ifndef FE_HMAC_SHA256_H
#define FE_HMAC_SHA256_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// SHA-256 上下文
typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    uint8_t data[64];
    uint32_t datalen;
} fe_sha256_ctx;

// HMAC-SHA256 上下文（封装两层 SHA-256 内垫）
typedef struct {
    fe_sha256_ctx inner;
    fe_sha256_ctx outer;
    uint8_t key_pad[64];
} fe_hmac_sha256_ctx;

void fe_sha256_init(fe_sha256_ctx *ctx);
void fe_sha256_update(fe_sha256_ctx *ctx, const uint8_t *data, size_t len);
void fe_sha256_final(fe_sha256_ctx *ctx, uint8_t hash[32]);

// 便捷：一次性计算 SHA-256
void fe_sha256(const uint8_t *data, size_t len, uint8_t out[32]);

// HMAC-SHA256：key 长度任意（<=64 直接使用，>64 先哈希）
void fe_hmac_sha256(const uint8_t *key, size_t key_len,
                    const uint8_t *msg, size_t msg_len,
                    uint8_t out[32]);

#ifdef __cplusplus
}
#endif

#endif // FE_HMAC_SHA256_H