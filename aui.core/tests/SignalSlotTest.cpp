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
// Created by Alex2772 on 2/3/2022.
//


#include <gtest/gtest.h>
#include <AUI/Common/AObject.h>
#include <AUI/Common/ASignal.h>
#include <AUI/Common/AString.h>
#include <AUI/Util/kAUI.h>
#include <gmock/gmock.h>

using namespace std::chrono_literals;

class Master: public AObject {
public:
    void broadcastMessage(const char* msg) {
        emit message(msg);
    }

signals:
    emits<AString> message;
};


class Slave: public AObject {
public:
    ~Slave() {
        die();
    }

    MOCK_METHOD(void, acceptMessage, (const AString& msg));
    MOCK_METHOD(void, acceptMessageNoArgs, ());
    MOCK_METHOD(void, die, ());
};

/**
 * Fixture.
 */
class SignalSlot : public testing::Test {
public:
    _<Master> master;
    _<Slave> slave;

    /**
     * Run once per test case before body.
     */
    void SetUp() override {
        Test::SetUp();
        master = _new<Master>();
    }

    /**
     * Run once per test case after body.
     */
    void TearDown() override {
        Test::TearDown();
        AThread::processMessages();
        AThread::sleep(500ms);

        master = nullptr;
        slave = nullptr;
    }
};

TEST_F(SignalSlot, Basic) {
    slave = _new<Slave>();
    AObject::connect(master->message, slot(slave)::acceptMessage);

    EXPECT_CALL(*slave, acceptMessage(AString("hello")));
    master->broadcastMessage("hello");
}


TEST_F(SignalSlot, BasicNoArgs) {
    slave = _new<Slave>();
    AObject::connect(master->message, slot(slave)::acceptMessageNoArgs);

    EXPECT_CALL(*slave, acceptMessageNoArgs);
    master->broadcastMessage("hello");
}

TEST_F(SignalSlot, Multithread) {
    slave = _new<Slave>();

    AObject::connect(master->message, slot(slave)::acceptMessage);

    EXPECT_CALL(*slave, acceptMessage(AString("hello")));
    auto t = async {
        master->broadcastMessage("hello");
    };
    t.wait();
}

TEST_F(SignalSlot, StackAllocatedObject) {
    testing::InSequence seq;
    Slave slave;

    AObject::connect(master->message, slot(slave)::acceptMessage);

    EXPECT_CALL(slave, acceptMessage(AString("hello"))).Times(1);
    master->broadcastMessage("hello");

    EXPECT_CALL(slave, die()).Times(1);
}

/**
 * Checks that the program is not crashed when one of the object is destroyed.
 */
TEST_F(SignalSlot, ObjectRemoval) {
    slave = _new<Slave>();
    {
        testing::InSequence s;
        AObject::connect(master->message, slot(slave)::acceptMessage); // imitate signal-slot relations
        EXPECT_CALL(*slave, die()).Times(1);
    }
}

/**
 * Checks for nested connection.
 */
TEST_F(SignalSlot, NestedConnection) {
    slave = _new<Slave>();
    AObject::connect(master->message, slave, [this, slave = slave.get()] (const AString& msg) {
        slave->acceptMessage(msg);
        AObject::connect(master->message, slave, [slave] (const AString& msg) {
            slave->acceptMessage(msg);
        });
    });

    EXPECT_CALL(*slave, acceptMessage(AString("hello"))).Times(3);
    master->broadcastMessage("hello");
    master->broadcastMessage("hello");
}

/**
 * Checks for disconnect functionality.
 */
TEST_F(SignalSlot, ObjectDisconnect1) {
    slave = _new<Slave>();
    AObject::connect(master->message, slave, [slave = slave.get()] (const AString& msg) {
        slave->acceptMessage(msg);
        AObject::disconnect();
    });

    EXPECT_CALL(*slave, acceptMessage(AString("hello"))).Times(1);
    master->broadcastMessage("hello");
    master->broadcastMessage("hello");
}

/**
 * Checks for disconnect functionality when one of the signals disconnected.
 */
TEST_F(SignalSlot, ObjectDisconnect2) {
    slave = _new<Slave>();

    bool called = false;
    AObject::connect(master->message, slave, [&, slave = slave.get()] (const AString& msg) {
        slave->acceptMessage(msg);
        called = true;
    });

    AObject::connect(master->message, slave, [slave = slave.get()] (const AString& msg) {
        slave->acceptMessage(msg);
        AObject::disconnect();
    });

    EXPECT_CALL(*slave, acceptMessage(AString("hello"))).Times(3);
    master->broadcastMessage("hello");
    called = false;
    master->broadcastMessage("hello");
    EXPECT_TRUE(called);
}


/**
 * Checks for both disconnect and nested connect.
 */
TEST_F(SignalSlot, ObjectNestedConnectWithDisconnect) {
    slave = _new<Slave>();

    bool called1 = false;
    bool called2 = false;

    AObject::connect(master->message, slave, [&] (const AString& msg) {
        AObject::disconnect();
        EXPECT_FALSE(called1);
        called1 = true;

        AObject::connect(master->message, slave, [&] (const AString& msg) {
            EXPECT_TRUE(called1);
            EXPECT_FALSE(called2);
            called2 = true;
        });
    });

    master->broadcastMessage("hello");
    master->broadcastMessage("hello");
    EXPECT_TRUE(called1);
    EXPECT_TRUE(called2);
}

/**
 * Destroys master in a signal handler
 */
TEST_F(SignalSlot, ObjectDestroyMasterInSignalHandler) {
    slave = _new<Slave>();
    {
        testing::InSequence s;
        AObject::connect(master->message, slave, [&] {
            master = nullptr;
        });
        master->broadcastMessage("hello");
        EXPECT_TRUE(master == nullptr);
    }
}

/**
 * Destroys slave in it's signal handler
 */
TEST_F(SignalSlot, ObjectDestroySlaveInSignalHandler) {
    slave = _new<Slave>();
    {
        testing::InSequence s;
        AObject::connect(master->message, slave, [&] {
            slave = nullptr;
        });
        master->broadcastMessage("hello");
        EXPECT_TRUE(slave == nullptr);
    }
}


TEST_F(SignalSlot, ObjectRemovalMultithread) {

    repeat(100) {

        class Slave2 : public AObject {
        public:
            Slave2(bool& called) : mCalled(called) {}

            void acceptMessage() {
                mCalled = true;
            }

        private:
            bool& mCalled;
        };

        bool called = false;

        auto slave2 = _new<Slave2>(called);

        AObject::connect(master->message, slot(slave2)::acceptMessage);

        auto task = async {
            master->broadcastMessage("hello");
        };


        for (;;) {
            AThread::processMessages();
            task.wait();
            if (called) {
                break;
            }
        }

        task = async {
            repeat(10000) master->broadcastMessage("hello");
        };
        task.wait();
        slave2 = nullptr; // delete slave; check for crash
        AThread::processMessages();
    }
}

