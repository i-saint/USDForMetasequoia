#include "pch.h"
#include "Test.h"

#include "mqsdk/sdk_Include.h"
#include "mqusd/Foundation/mqusdUtils.h"

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

TestCase(Test_Angle)
{
    auto q = mu::rotate_zxy(float3{ 15.0f, 30.0f, 45.0f });
    auto a = mqusd::to_angle(q);
    auto q2 = mqusd::to_quat(a);
    Expect(mu::near_equal(q, q2) || mu::near_equal(q, -q2));
}
