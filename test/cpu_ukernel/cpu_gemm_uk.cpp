#include <iostream>
#include <initializer_list>
#include <cstdlib>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <tuple>
#include <memory>
#include <chrono>
#include <half.hpp>
#include "config.hpp"
#include "print.hpp"
#include "cpuid.hpp"
#include "threadwise_gemm_avx2.hpp"

#define ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE(FA, FB, FC, TA, TB, NT)   \
    ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 6, 16, TA, TB, NT>,     \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 5, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 4, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 3, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 2, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 1, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 6, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 5, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 4, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 3, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 2, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC, 1, 8, TA, TB, NT>

//#define ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE(FA, FB, FC, TA, TB, NT)  \
//     ck::cpu::ThreadwiseGemmAvx2_MxN_6x16<FA, FB, FC,  6, 16,  TA,  TB,  NT>

#define ITERATE_THREAD_GEMM_AVX2_MXN_4X24_INSTANCE(FA, FB, FC, TA, TB, NT)   \
    ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 4, 24, TA, TB, NT>,     \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 3, 24, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 2, 24, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 1, 24, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 4, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 3, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 2, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 1, 16, TA, TB, NT>, \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 4, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 3, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 2, 8, TA, TB, NT>,  \
        ck::cpu::ThreadwiseGemmAvx2_MxN_4x24<FA, FB, FC, 1, 8, TA, TB, NT>

using Row = ck::tensor_layout::gemm::RowMajor;
using Col = ck::tensor_layout::gemm::ColumnMajor;

// using AType = half_float::half;
// using BType = half_float::half;
using AType = float;
using BType = float;
using CType = float;

template <typename ALayout, typename BLayout>
using thread_gemm_avx2_mxn_6x16_instances = std::tuple<
    // clang-format off
    //                                        FloatA FloatB FloatC  ALayout  BLayout NTStore
    ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE( AType, BType, CType, ALayout, BLayout, false)

    // ITERATE_THREAD_GEMM_AVX2_MXN_6X16_INSTANCE(AType, BType, CType,    ALayout,    BLayout, false)
    // clang-format on
    >;

template <typename ALayout, typename BLayout>
using thread_gemm_avx2_mxn_4x24_instances = std::tuple<
    // clang-format off
    //                                        FloatA FloatB FloatC  ALayout  BLayout NTStore
    ITERATE_THREAD_GEMM_AVX2_MXN_4X24_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_4X24_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_4X24_INSTANCE( AType, BType, CType, ALayout, BLayout, false),
    ITERATE_THREAD_GEMM_AVX2_MXN_4X24_INSTANCE( AType, BType, CType, ALayout, BLayout, false)
    // clang-format on
    >;

void dump_cache_hierarchy()
{
    auto dump_cache_type = [&](const ck::cpu::cpuid_cache_type& type) {
        if(type == ck::cpu::cpuid_cache_type_dcache)
            printf("data cache");
        else if(type == ck::cpu::cpuid_cache_type_icache)
            printf("inst cache");
        else if(type == ck::cpu::cpuid_cache_type_unified)
            printf("unif cache");
    };
    auto dump_cache_detail = [&](const ck::cpu::cpuid_cache_detail& detail) {
        dump_cache_type(static_cast<const ck::cpu::cpuid_cache_type>(detail.type));
        printf(" size:%u, cache_line:%u, associativity:%u, sets:%u, partitions:%u, shared by "
               "procs:%u(%u)\n",
               detail.size,
               detail.cache_line_size,
               detail.associativity,
               detail.sets,
               detail.partitions,
               detail.shared_by_procs,
               detail.cores_per_socket);
    };

    ck::cpu::cpuid_cache_hierarchy cache = ck::cpu::cpuid_query_cache();
    if(cache.l1d.size != 0)
    {
        printf("l1 ");
        dump_cache_detail(cache.l1d);
    }
    if(cache.l1i.size != 0)
    {
        printf("l1 ");
        dump_cache_detail(cache.l1i);
    }
    if(cache.l2.size != 0)
    {
        printf("l2 ");
        dump_cache_detail(cache.l2);
    }
    if(cache.l3.size != 0)
    {
        printf("l3 ");
        dump_cache_detail(cache.l3);
    }
    if(cache.l4.size != 0)
    {
        printf("l4 ");
        dump_cache_detail(cache.l4);
    }
}

void* __aligned_malloc(size_t required_bytes, size_t alignment)
{
    if(alignment == 0 || (alignment & (alignment - 1))) // check pow of 2
        return nullptr;
    void* p1;  // original block
    void** p2; // aligned block
    int offset = alignment - 1 + sizeof(void*);
    if((p1 = malloc(required_bytes + offset)) == nullptr)
    {
        return nullptr;
    }
    p2     = reinterpret_cast<void**>((reinterpret_cast<size_t>(p1) + offset) & ~(alignment - 1));
    p2[-1] = p1;
    return p2;
}

void __aligned_free(void* p) { free((reinterpret_cast<void**>(p))[-1]); }

template <typename T>
void rand_vector(T* v, int elem)
{
    int i;

    static int flag = 0;
    if(!flag)
    {
        srand(time(nullptr));
        flag = 1;
    }

    for(i = 0; i < elem; i++)
    {
        v[i] = (static_cast<T>(rand() % 100)) / 100.0f;
    }
}

bool valid_vector(const float* ref, const float* rhs, uint32_t elem)
{
    float rtol   = 1e-5;
    float atol   = 1e-8;
    uint32_t err = 0;
    for(uint32_t i = 0; i < elem; i++)
    {
        float diff = std::abs(ref[i] - rhs[i]);
        if(diff > atol + rtol * std::abs(ref[i]))
        {
            printf("diff at %u, ref:%f, rhs:%f\n", i, ref[i], rhs[i]);
            err++;
        }
    }

    return err == 0;
}

template <typename FloatA, typename FloatB, typename ALayout, typename BLayout>
void ref_cpu_gemm_uk(
    const FloatA* a, const FloatB* b, float* c, float alpha, uint32_t m, uint32_t n, uint32_t k)
{
    auto a_offset = [&](uint32_t im, uint32_t ik) {
        if constexpr(std::is_same<Row, ALayout>::value)
        {
            return im * k + ik;
        }
        else
        {
            return ik * m + im;
        }
    };

    auto b_offset = [&](uint32_t ik, uint32_t in) {
        if constexpr(std::is_same<Row, BLayout>::value)
        {
            return ik * n + in;
        }
        else
        {
            // n*k*n8
            return (in / 8) * k * 8 + ik * 8 + in % 8;
        }
    };

    auto c_offset = [&](uint32_t im, uint32_t in) { return im * n + in; };

    for(uint32_t im = 0; im < m; im++)
    {
        for(uint32_t in = 0; in < n; in++)
        {
            float acc = .0f;
            for(uint32_t ik = 0; ik < k; ik++)
            {
                acc += static_cast<float>(a[a_offset(im, ik)]) *
                       static_cast<float>(b[b_offset(ik, in)]);
            }
            acc *= alpha;
            c[c_offset(im, in)] = acc;
        }
    }
}

template <typename FloatA, typename FloatB, typename ALayout, typename BLayout, typename ukenrel_t>
void test_ukernel(ukenrel_t uk,
                  FloatA* mat_a,
                  FloatB* mat_b,
                  float* mat_c,
                  float alpha,
                  uint32_t m,
                  uint32_t n,
                  uint32_t k)
{
    ck::cpu::ThreadwiseGemmParam param;
    param.p_a   = mat_a;
    param.p_b   = mat_b;
    param.p_c   = mat_c;
    param.Kr    = k;
    param.lda   = (std::is_same<Row, ALayout>::value ? k : m) * sizeof(FloatA);
    param.ldb   = (std::is_same<Row, BLayout>::value ? n : k * 8) * sizeof(FloatB);
    param.ldc   = n * sizeof(float);
    param.alpha = alpha;

    auto invoke_uk = [&]() {
        if constexpr(std::is_same<Row, ALayout>::value && std::is_same<Row, BLayout>::value)
        {
            assert(m % uk.Mr_ == 0 && n == uk.Nr_);
            FloatA* p_a = mat_a;
            float* p_c  = mat_c;
            param.p_a   = p_a;
            param.p_c   = p_c;
            for(uint32_t i_m = 0; i_m < m; i_m += uk.Mr_)
            {
                uk.Run(&param);
                p_a += uk.Mr_ * k;
                p_c += uk.Mr_ * n;
                param.p_a = p_a;
                param.p_c = p_c;
            }
        }
        else if constexpr(std::is_same<Row, ALayout>::value && std::is_same<Col, BLayout>::value)
        {
            assert(m % uk.Mr_ == 0 && n % uk.Nr_ == 0);
            FloatA* p_a = mat_a;
            float* p_c  = mat_c;
            param.p_a   = p_a;
            param.p_b   = mat_b;
            param.p_c   = p_c;
            for(uint32_t i_m = 0; i_m < m; i_m += uk.Mr_)
            {
                float* p_c_n  = p_c;
                FloatB* p_b_n = mat_b;
                for(uint32_t i_n = 0; i_n < n; i_n += uk.Nr_)
                {
                    uk.Run(&param);
                    p_b_n += uk.Nr_ * k; // Nr_/8*k*8
                    p_c_n += uk.Nr_;
                    param.p_b = p_b_n;
                    param.p_c = p_c_n;
                }
                p_a += uk.Mr_ * k;
                p_c += uk.Mr_ * n;
                param.p_a = p_a;
                param.p_b = mat_b;
                param.p_c = p_c;
            }
        }
        else if constexpr(std::is_same<Col, ALayout>::value && std::is_same<Row, BLayout>::value)
        {
            assert(m == uk.Mr_ && n == uk.Nr_);
            uk.Run(&param);
        }
        else
        {
            assert(m % uk.Mr_ == 0 && n % uk.Nr_ == 0);
            FloatB* p_b = mat_b;
            float* p_c  = mat_c;
            param.p_b   = p_b;
            param.p_c   = p_c;
            for(uint32_t i_n = 0; i_n < n; i_n += uk.Nr_)
            {
                uk.Run(&param);
                p_b += uk.Nr_ * k; // Nr_/8*k*8
                p_c += uk.Nr_;
                param.p_b = p_b;
                param.p_c = p_c;
            }
        }
    };

    printf("gemm_uk_%dx%d_%c%c: ", uk.Mr_, uk.Nr_, ALayout::name[0], BLayout::name[0]);
    fflush(stdout);
    // printf("%s: ", typeid(uk).name());fflush(stdout);
    memset(mat_c, 0, m * n * sizeof(float));

    int repeat = 7e10 / (2 * m * n * k);

    for(int i = 0; i < (repeat / 5); i++)
    {
        invoke_uk();
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    for(int i = 0; i < repeat; i++)
    {
        invoke_uk();
    }
    auto t1 = std::chrono::high_resolution_clock::now();

    double us = static_cast<double>(
                    std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count()) /
                repeat;
    double gflops = static_cast<double>(2 * m * n * k) * 1e-3 / us;

    memset(mat_c, 0, m * n * sizeof(float));
    invoke_uk();

    printf("m:%u, n:%u, k:%u, alpha:%f, cost:%lfus, GFLOPS:%lf, ", m, n, k, alpha, us, gflops);
    fflush(stdout);
}

// implement small ukernel on L1
template <typename FloatA, typename FloatB, typename ALayout, typename BLayout>
void test_cpu_ukernel(float alpha, uint32_t m, uint32_t n, uint32_t k)
{
    FloatA* mat_a = reinterpret_cast<FloatA*>(__aligned_malloc(m * k * sizeof(FloatA), 32));
    FloatB* mat_b = reinterpret_cast<FloatB*>(__aligned_malloc(k * n * sizeof(FloatB), 32));
    float* mat_c  = reinterpret_cast<float*>(__aligned_malloc(m * n * sizeof(float), 32));

    float* mat_c_ref = reinterpret_cast<float*>(__aligned_malloc(m * n * sizeof(float), 32));
    memset(mat_c_ref, 0, m * n * sizeof(float));

    rand_vector(mat_a, m * k);
    rand_vector(mat_b, k * n);

    ref_cpu_gemm_uk<FloatA, FloatB, ALayout, BLayout>(mat_a, mat_b, mat_c_ref, alpha, m, n, k);

    using thread_gemm_instance = thread_gemm_avx2_mxn_6x16_instances<ALayout, BLayout>;
    // using thread_gemm_instance = thread_gemm_avx2_mxn_4x24_instances<ALayout, BLayout>;
    bool found = false;

    ck::static_for<0, std::tuple_size_v<thread_gemm_instance>, 1>{}([&](auto i) {
        using uk_type = std::tuple_element_t<i, thread_gemm_instance>;
        if(m % uk_type::Mr_ != 0 || n % uk_type::Nr_ != 0)
            return;
        if((m != uk_type::Mr_ && std::is_same<typename uk_type::ALayout_, Col>::value) ||
           (n != uk_type::Nr_ && std::is_same<typename uk_type::BLayout_, Row>::value))
            // only k is the fast changing dim of A/B can we do muldiplt m, n
            return;

        if(found)
            return;

        test_ukernel<FloatA, FloatB, ALayout, BLayout>(
            uk_type{}, mat_a, mat_b, mat_c, alpha, m, n, k);

        bool is_valid = valid_vector(mat_c_ref, mat_c, m * n);
        printf("vald:%s\n", is_valid ? "y" : "n");
        found = true;
    });

    __aligned_free(mat_a);
    __aligned_free(mat_b);
    __aligned_free(mat_c);
    __aligned_free(mat_c_ref);
}

int main(int argc, char** argv)
{
    int m       = 6;
    int n       = 16;
    int k       = 64;
    float alpha = 1.0f;
    if(argc > 3)
    {
        m = std::atoi(argv[1]);
        n = std::atoi(argv[2]);
        k = std::atoi(argv[3]);
    }
    if(argc > 4)
    {
        alpha = std::atof(argv[4]);
    }
    dump_cache_hierarchy();
    test_cpu_ukernel<AType, BType, Row, Row>(alpha, m, n, k);
    test_cpu_ukernel<AType, BType, Row, Col>(alpha, m, n, k);
    test_cpu_ukernel<AType, BType, Col, Row>(alpha, m, n, k);
    test_cpu_ukernel<AType, BType, Col, Col>(alpha, m, n, k);
}