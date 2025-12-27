#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../src/worker.hpp"
#include "../src/protocol.hpp"
#include <vector>
#include <cstring>
#include <numeric>

using namespace testing;

// --- MOCKS ---
class MockNetwork : public INetwork {
public:
    MOCK_METHOD(void, connect, (const std::string&, int), (override));
    MOCK_METHOD(void, disconnect, (), (override));
    MOCK_METHOD(void, send_all, (const std::string&), (override));
    MOCK_METHOD(std::vector<uint8_t>, recv_exact, (size_t), (override));
    MOCK_METHOD(std::string, recv_until, (const std::string&), (override));
};

class MockFileSystem : public IFileSystem {
public:
    MOCK_METHOD(void, write_line, (const std::string&), (override));
};

// --- HELPER FUNCTIONS ---
void push_int64(std::vector<uint8_t>& buf, int64_t val) {
    for(int i=7; i>=0; --i) buf.push_back((val >> (i*8)) & 0xFF);
}
void push_int32(std::vector<uint8_t>& buf, int32_t val) {
    for(int i=3; i>=0; --i) buf.push_back((val >> (i*8)) & 0xFF);
}
void push_int16(std::vector<uint8_t>& buf, int16_t val) {
    for(int i=1; i>=0; --i) buf.push_back((val >> (i*8)) & 0xFF);
}
void push_float(std::vector<uint8_t>& buf, float val) {
    uint32_t temp;
    std::memcpy(&temp, &val, sizeof(float));
    for(int i=3; i>=0; --i) buf.push_back((temp >> (i*8)) & 0xFF);
}

// Правильный расчет суммы для ГЕНЕРАЦИИ пакета (считаем все байты)
uint8_t calc_sum_for_test(const std::vector<uint8_t>& data) {
    int sum = 0;
    for (uint8_t b : data) sum = (sum + b) % 256;
    return static_cast<uint8_t>(sum);
}

// --- PROTOCOL TESTS ---

TEST(Protocol, Checksum) {
    // Входящий пакет: 1, 2, 3. Функция должна посчитать 1+2 = 3.
    // Последний байт (3) считается контрольной суммой и игнорируется.
    std::vector<uint8_t> d = {1, 2, 3};
    EXPECT_EQ(Protocol::calculate_checksum(d), 3);
}

TEST(Protocol, ParseInt64) {
    std::vector<uint8_t> buf;
    push_int64(buf, 1234567890123456789LL);
    EXPECT_EQ(Protocol::parse_int64(buf.data()), 1234567890123456789LL);
    buf.clear(); push_int64(buf, -1);
    EXPECT_EQ(Protocol::parse_int64(buf.data()), -1);
}

TEST(Protocol, ParseInt32) {
    std::vector<uint8_t> buf;
    push_int32(buf, 2147483647);
    EXPECT_EQ(Protocol::parse_int32(buf.data()), 2147483647);
    buf.clear(); push_int32(buf, -500);
    EXPECT_EQ(Protocol::parse_int32(buf.data()), -500);
}

TEST(Protocol, ParseInt16) {
    std::vector<uint8_t> buf;
    push_int16(buf, 32000);
    EXPECT_EQ(Protocol::parse_int16(buf.data()), 32000);
}

TEST(Protocol, ParseFloat) {
    std::vector<uint8_t> buf;
    push_float(buf, 123.456f);
    EXPECT_FLOAT_EQ(Protocol::parse_float(buf.data()), 123.456f);
}

TEST(Protocol, FormatTime) {
    int64_t ts = 1609459200000000LL;
    EXPECT_EQ(Protocol::format_time(ts), "2021-01-01 00:00:00");
}

// --- WORKER TESTS ---

TEST(Worker, Type1_Success) {
    auto net = std::make_shared<NiceMock<MockNetwork>>();
    auto fs = std::make_shared<NiceMock<MockFileSystem>>();
    DataWorker worker(net, fs, "host", 1, 1);

    std::vector<uint8_t> pkt;
    push_int64(pkt, 1000);
    push_float(pkt, 25.5f);
    push_int16(pkt, 760);
    // Добавляем сумму всех предыдущих байт
    pkt.push_back(calc_sum_for_test(pkt));

    {
        InSequence s;
        EXPECT_CALL(*net, connect("host", 1));
        EXPECT_CALL(*net, send_all("isu_pt"));
        EXPECT_CALL(*net, recv_until("granted")).WillOnce(Return("granted"));

        // Цикл данных 1 (Успех)
        EXPECT_CALL(*net, send_all("get"));
        EXPECT_CALL(*net, recv_exact(15)).WillOnce(Return(pkt));
        EXPECT_CALL(*fs, write_line(HasSubstr("Temp: 25.50")));

        // Цикл данных 2 (Остановка теста)
        EXPECT_CALL(*net, send_all("get"))
            .WillOnce(InvokeWithoutArgs([&](){
                worker.stop();
                throw std::runtime_error("Stop");
            }));

        EXPECT_CALL(*net, disconnect());
    }
    worker.run_loop();
}

TEST(Worker, Type2_Success) {
    auto net = std::make_shared<NiceMock<MockNetwork>>();
    auto fs = std::make_shared<NiceMock<MockFileSystem>>();
    DataWorker worker(net, fs, "host", 2, 2);

    std::vector<uint8_t> pkt;
    push_int64(pkt, 1000);
    push_int32(pkt, 10);
    push_int32(pkt, -20);
    push_int32(pkt, 30);
    pkt.push_back(calc_sum_for_test(pkt));

    {
        InSequence s;
        EXPECT_CALL(*net, connect(_, _));
        EXPECT_CALL(*net, send_all("isu_pt"));
        EXPECT_CALL(*net, recv_until("granted")).WillOnce(Return("granted"));

        EXPECT_CALL(*net, send_all("get"));
        EXPECT_CALL(*net, recv_exact(21)).WillOnce(Return(pkt));
        EXPECT_CALL(*fs, write_line(AllOf(HasSubstr("Y: -20"), HasSubstr("Z: 30"))));

        EXPECT_CALL(*net, send_all("get"))
            .WillOnce(InvokeWithoutArgs([&](){
                worker.stop();
                throw std::runtime_error("Stop");
            }));

        EXPECT_CALL(*net, disconnect());
    }
    worker.run_loop();
}

TEST(Worker, ChecksumMismatch) {
    auto net = std::make_shared<NiceMock<MockNetwork>>();
    auto fs = std::make_shared<StrictMock<MockFileSystem>>();
    DataWorker worker(net, fs, "host", 1, 1);

    std::vector<uint8_t> pkt(15, 0);
    pkt.back() = 55; // Заведомо неверная сумма

    {
        InSequence s;
        EXPECT_CALL(*net, connect(_, _));
        EXPECT_CALL(*net, send_all("isu_pt"));
        EXPECT_CALL(*net, recv_until("granted")).WillOnce(Return("granted"));

        EXPECT_CALL(*net, send_all("get"));
        EXPECT_CALL(*net, recv_exact(15)).WillOnce(Return(pkt));

        // write_line не вызывается

        EXPECT_CALL(*net, send_all("get"))
            .WillOnce(InvokeWithoutArgs([&](){
                worker.stop();
                throw std::runtime_error("Stop");
            }));
        EXPECT_CALL(*net, disconnect());
    }
    worker.run_loop();
}

TEST(Worker, ConnectionFailure) {
    auto net = std::make_shared<NiceMock<MockNetwork>>();
    auto fs = std::make_shared<NiceMock<MockFileSystem>>();
    DataWorker worker(net, fs, "host", 1, 1);

    EXPECT_CALL(*net, connect("host", 1))
        .WillOnce(Throw(std::runtime_error("Network Unreachable")));

    EXPECT_CALL(*net, disconnect()).WillOnce(
        InvokeWithoutArgs([&](){ worker.stop(); })
    );

    worker.run_loop();
}

TEST(Worker, HandshakeFailure) {
    auto net = std::make_shared<NiceMock<MockNetwork>>();
    auto fs = std::make_shared<NiceMock<MockFileSystem>>();
    DataWorker worker(net, fs, "host", 1, 1);

    {
        InSequence s;
        EXPECT_CALL(*net, connect(_, _));
        EXPECT_CALL(*net, send_all("isu_pt"));

        EXPECT_CALL(*net, recv_until("granted"))
            .WillOnce(Throw(std::runtime_error("Timeout")));

        EXPECT_CALL(*net, disconnect()).WillOnce(
            InvokeWithoutArgs([&](){ worker.stop(); })
        );
    }
    worker.run_loop();
}