#include "pch.h"
#include "Test.h"

#include "MeshUtils/MeshUtils.h"
#include "SceneGraph/SceneGraph.h"
#include "SceneGraph/sgSerializationImpl.h"

using sg::serializer;
using sg::deserializer;

namespace test {

class BaseData
{
public:
    virtual ~BaseData();
    virtual void serialize(serializer& s) const;
    virtual void deserialize(deserializer& d);

    void makeData();
    void print();

    std::vector<int> m_vector_data;   // std::vector
    std::list<int> m_list_data;       // std::list
    std::shared_ptr<BaseData> m_data; // std::shared_ptr to polymorphic type
    BaseData* m_pointer = nullptr;
};

class DerivedData : public BaseData
{
using super = BaseData;
public:
    void serialize(serializer& s) const override;
    void deserialize(deserializer& d) override;

    std::map<std::string, std::set<int>> m_map_set; // std::map, std::string, std::set
};

} // namespace test {

sgSerializable(test::BaseData)
sgRegisterType(test::BaseData)
sgRegisterType(test::DerivedData)


namespace test {

BaseData::~BaseData()
{
}

#define EachMember(F) F(m_vector_data) F(m_list_data) F(m_data) F(m_pointer)

void BaseData::serialize(serializer& s) const
{
    EachMember(sgWrite);
}

void BaseData::deserialize(deserializer& d)
{
    EachMember(sgRead);
}

#undef EachMember

void BaseData::makeData()
{
    m_data = std::make_shared<DerivedData>();
    auto& dst = static_cast<DerivedData&>(*m_data);
    m_pointer = &dst;

    dst.m_vector_data.resize(128);
    std::iota(dst.m_vector_data.begin(), dst.m_vector_data.end(), 0);

    dst.m_list_data.resize(128);
    std::iota(dst.m_list_data.begin(), dst.m_list_data.end(), 128);

    char buf[128];
    for (int i = 0; i < 32; ++i) {
        sprintf(buf, "data %02d", i);
        auto& set = dst.m_map_set[buf];
        for (int j = 0; j < 32; ++j)
            set.insert(i * j);
    }
}

void BaseData::print()
{
    if (m_data) {
        auto& src = *m_data;

        printf("m_vector_data: ");
        for (auto v : src.m_vector_data)
            printf("%d ", v);
        printf("\n");

        printf("m_list_data: ");
        for (auto v : src.m_list_data)
            printf("%d ", v);
        printf("\n");
    }

    if (auto* derived = dynamic_cast<DerivedData*>(m_data.get())) {
        auto& src = *derived;

        printf("m_map_set:\n");
        for (auto kvp : src.m_map_set) {
            printf("  \"%s\": ", kvp.first.c_str());
            for (auto v : kvp.second)
                printf("%d ", v);
            printf("\n");
        }
        printf("\n");
    }
}


#define EachMember(F) F(m_map_set)

void DerivedData::serialize(serializer& s) const
{
    super::serialize(s);
    EachMember(sgWrite);
}

void DerivedData::deserialize(deserializer& d)
{
    super::deserialize(d);
    EachMember(sgRead);
}
#undef EachMember

} // namespace test


TestCase(Test_Serialization)
{
    std::string buffer;
    {
        std::stringstream ss;
        sg::serializer s(ss);

        test::BaseData data;
        data.makeData();
        data.serialize(s);

        buffer = ss.str();
    }

    {
        std::stringstream ss(buffer);
        sg::deserializer d(ss);

        test::BaseData data;
        data.deserialize(d);
        data.print();
    }
}
