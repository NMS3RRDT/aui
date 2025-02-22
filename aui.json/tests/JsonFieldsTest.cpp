// AUI Framework - Declarative UI toolkit for modern C++20
// Copyright (C) 2020-2023 Alex2772
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library. If not, see <http://www.gnu.org/licenses/>.

//
// Created by alex2 on 30.08.2020.
//

#include <gtest/gtest.h>
#include <AUI/Common/AString.h>
#include <AUI/Json/AJson.h>
#include <AUI/Json/AJson.h>
#include <AUI/Traits/parameter_pack.h>
#include <AUI/Traits/members.h>

// ORM data class
struct Data2 {
    AVector<int> values;
    int i;

    bool operator==(const Data2& o) const {
        return std::tie(values, i) == std::tie(o.values, o.i);
    }
};

AJSON_FIELDS(Data2,
        (values, "values")
        (i, "i")
)

std::ostream& operator<<(std::ostream& o, const Data2& d) {
    o << '[';
    for (auto& v : d.values) {
        o << v << ", ";
    }
    o << "]," << d.i;
    return o;
}

TEST(Json, FieldsTestBasic)
{
    // arrange
    Data2 d = { {1, 2, 3}, 228 };

    // check for serialization
    auto jsonObject = aui::to_json(d);
    ASSERT_EQ(AJson::toString(jsonObject), R"({"i":228,"values":[1,2,3]})");

    auto d2 = aui::from_json<Data2>(jsonObject);
    ASSERT_EQ(d, d2);

    EXPECT_THROW(aui::from_json<Data2>(AJson::fromString(R"({"i":228})") /* no "values" field present */), AJsonException);
}

struct DataOptional {
    int v1;
    int v2;
};

AJSON_FIELDS(DataOptional,
             (v1, "v1")
             (v2, "v2", AJsonFieldFlags::OPTIONAL))

TEST(Json, FieldsTestOptional)
{
    // should not throw an exception since v2 is optional
    DataOptional dst { 1, 2 };
    EXPECT_NO_THROW(aui::from_json<DataOptional>(AJson::fromString(R"({"v1":228})"), dst));
    EXPECT_EQ(dst.v1, 228); // should have modified presented value
    EXPECT_EQ(dst.v2, 2); // but should have kept previous value

    // it should not ignore the value when it present
    auto d2 = aui::from_json<DataOptional>(AJson::fromString(R"({"v1":228, "v2":229})"));
    EXPECT_EQ(d2.v1, 228);
    EXPECT_EQ(d2.v2, 229);

    // here it should throw an exception since v1 is not optional
    EXPECT_THROW(aui::from_json<DataOptional>(AJson::fromString(R"({"v2":228})")), AJsonException);
}
