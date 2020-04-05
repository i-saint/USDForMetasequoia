#include "pch.h"
#include "Test.h"

#include "MeshUtils/muSIMDConfig.h"
#include "Plugins/mqsdk/sdk_Include.h"
#include "Plugins/mqCommon/mqusdUtils.h"

using namespace mu;

TestCase(Test_PipeStream)
{
    const char cmd_ls[] =
#ifdef _WIN32
        "dir";
#else
        "ls";
#endif

    {
        // text mode
        std::cout << "\ntext mode:\n";
        PipeStream pstream;
        if (pstream.open(cmd_ls, std::ios::in)) {
            std::string line;
            while (std::getline(pstream, line))
                std::cout << line << std::endl;
        }
    }
    {
        // binary mode
        std::cout << "\nbinary mode:\n";
        PipeStream pstream;
        if (pstream.open(cmd_ls, std::ios::in | std::ios::binary)) {
            std::vector<char> buf;
            buf.resize(1024 * 16);
            pstream.read(buf.data(), buf.size());
            buf.resize(pstream.gcount());
            std::cout.write(buf.data(), buf.size());
        }
    }
}

TestCase(Test_GetCurrentModuleDirectory)
{
    auto dir = GetCurrentModuleDirectory();
    std::cout << dir << std::endl;
}

TestCase(Test_MulPoints)
{
    const int num_data = 65536;
    const int num_try = 128;

    RawVector<float3> src, dst1, dst2;
    src.resize(num_data);
    dst1.resize(num_data);
    dst2.resize(num_data);

    float4x4 matrix = transform({ 1.0f, 2.0f, 4.0f }, rotate_y(45.0f), {2.0f, 2.0f, 2.0f});

    for (int i = 0; i < num_data; ++i) {
        src[i] = { (float)i*0.1f, (float)i*0.05f, (float)i*0.025f };
    }

    Print(
        "    num_data: %d\n"
        "    num_try: %d\n",
        num_data,
        num_try);

    int offset = 1;

    TestScope("MulPoints C++", [&]() {
        MulPoints_Generic(matrix, src.data() + offset, dst1.data() + offset, num_data - offset);
    }, num_try);
#ifdef muSIMD_MulPoints3
    TestScope("MulPoints ISPC", [&]() {
        MulPoints_ISPC(matrix, src.data() + offset, dst2.data() + offset, num_data - offset);
    }, num_try);
    if (!NearEqual(dst1.data(), dst2.data(), num_data, 1e-3f)) {
        Print("    *** validation failed ***\n");
    }
#endif

    TestScope("MulVectors C++", [&]() {
        MulVectors_Generic(matrix, src.data() + offset, dst1.data() + offset, num_data - offset);
    }, num_try);
#ifdef muSIMD_MulVectors3
    TestScope("MulVectors ISPC", [&]() {
        MulVectors_ISPC(matrix, src.data() + offset, dst2.data() + offset, num_data - offset);
    }, num_try);
    if (!NearEqual(dst1.data() + offset, dst2.data() + offset, num_data - offset)) {
        Print("    *** validation failed ***\n");
    }
#endif
}

TestCase(Test_Angle)
{
    auto q = mu::rotate_zxy(float3{ 15.0f, 30.0f, 45.0f });
    auto a = mqusd::to_angle(q);
    auto q2 = mqusd::to_quat(a);
    Expect(mu::near_equal(q, q2) || mu::near_equal(q, -q2));
}


template<class T>
void Test_Limits_Impl()
{
    auto _0 = mu::inf<T>();
    auto _1 = mu::nan<T>();
    auto _2 = mu::min<T>();
    auto _3 = mu::max<T>();
    auto _4 = mu::epsilon<T>();
    auto _5 = mu::isnan(_0);
    auto _6 = mu::isnan(_1);
    auto _7 = mu::isinf(_0);
    auto _8 = mu::isinf(_1);
}

TestCase(Test_Limits)
{
    Test_Limits_Impl<float2>();
    Test_Limits_Impl<float3>();
    Test_Limits_Impl<float4>();
    Test_Limits_Impl<quatf>();
    Test_Limits_Impl<float2x2>();
    Test_Limits_Impl<float3x3>();
    Test_Limits_Impl<float4x4>();

    Test_Limits_Impl<double2>();
    Test_Limits_Impl<double3>();
    Test_Limits_Impl<double4>();
    Test_Limits_Impl<quatd>();
    Test_Limits_Impl<double2x2>();
    Test_Limits_Impl<double3x3>();
    Test_Limits_Impl<double4x4>();
}
