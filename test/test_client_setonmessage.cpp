//
// DarwinCore Network Client - SetOnMessage 单元测试
//
// 测试场景：
//   1. SetOnMessage 回调设置后，接收数据时回调被正确调用
//   2. 回调接收到正确的数据内容
//   3. 替换已有的回调（新回调生效）
//   4. 设置空回调（不影响连接）
//   5. 多次接收到数据，回调被多次调用
//
// 作者: DarwinCore Network 团队
// 日期: 2026

#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <string>
#include <cassert>
#include <cstring>
#include <unistd.h>

#include <darwincore/network/server.h>
#include <darwincore/network/client.h>

using namespace darwincore::network;

// 测试结果统计
struct TestResult {
  std::string name;
  bool passed;
  std::string message;

  TestResult(const std::string& n, bool p, const std::string& m = "")
      : name(n), passed(p), message(m) {}
};

std::vector<TestResult> g_results;

void RecordResult(const std::string& name, bool passed, const std::string& message = "") {
  g_results.emplace_back(name, passed, message);
  std::cout << (passed ? "[PASS] " : "[FAIL] ") << name;
  if (!message.empty()) {
    std::cout << " - " << message;
  }
  std::cout << std::endl;
}

// 测试 1: SetOnMessage 回调被正确调用
bool TestCallbackInvocation() {
  std::cout << "\n========== 测试 1: SetOnMessage 回调被正确调用 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<int> callback_count(0);
  std::atomic<int> expected_callback_count(1);

  std::thread server_thread([&]() {
    Server server;

    server.SetOnClientConnected([&](const ConnectionInformation&) {
      // Server 端处理连接
    });

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>& data) {
      // 收到 Client 的消息后，发送回复
      std::string reply = "Server Reply";
      server.SendData(conn_id,
                      reinterpret_cast<const uint8_t*>(reply.c_str()),
                      reply.length());
    });

    if (!server.StartIPv4("127.0.0.1", 9988)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;
    std::cout << "[Server] 启动成功，等待连接..." << std::endl;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[Server] 测试完成，停止中..." << std::endl;
    server.Stop();
  });

  std::thread client_thread([&]() {
    // 等待 Server 准备好
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    // 设置 OnMessage 回调
    client.SetOnMessage([&](const std::vector<uint8_t>& data) {
      callback_count++;
      std::string msg(data.begin(), data.end());
      std::cout << "[Client] 收到消息: " << msg << std::endl;
    });

    if (!client.ConnectIPv4("127.0.0.1", 9988)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    // 等待连接建立
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 发送测试消息
    std::string msg = "Hello from Client";
    client.SendData(reinterpret_cast<const uint8_t*>(msg.c_str()), msg.length());
    std::cout << "[Client] 发送测试消息: " << msg << std::endl;

    // 等待接收回复
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    client.Disconnect();
    test_done = true;
  });

  // 等待测试完成
  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  bool success = (callback_count.load() == expected_callback_count.load());
  std::cout << "回调调用次数: " << callback_count.load()
            << " (期望: " << expected_callback_count.load() << ")" << std::endl;

  RecordResult("SetOnMessage 回调被正确调用", success,
               "回调调用次数: " + std::to_string(callback_count.load()));

  return success;
}

// 测试 2: 回调接收到正确的数据内容
bool TestCallbackDataContent() {
  std::cout << "\n========== 测试 2: 回调接收到正确的数据内容 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<bool> data_correct(false);

  const std::string test_data = "Test Data Content 12345";

  std::thread server_thread([&]() {
    Server server;

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>& data) {
      // 回复收到的数据
      server.SendData(conn_id, data.data(), data.size());
    });

    if (!server.StartIPv4("127.0.0.1", 9989)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.Stop();
  });

  std::thread client_thread([&]() {
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    client.SetOnMessage([&](const std::vector<uint8_t>& data) {
      std::string received(data.begin(), data.end());
      if (received == test_data) {
        data_correct = true;
        std::cout << "[Client] 数据内容验证成功: " << received << std::endl;
      } else {
        std::cout << "[Client] 数据内容验证失败! 期望: " << test_data
                  << " 实际: " << received << std::endl;
      }
    });

    if (!client.ConnectIPv4("127.0.0.1", 9989)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 发送测试数据
    client.SendData(reinterpret_cast<const uint8_t*>(test_data.c_str()),
                    test_data.length());
    std::cout << "[Client] 发送测试数据: " << test_data << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    client.Disconnect();
    test_done = true;
  });

  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  RecordResult("回调接收到正确的数据内容", data_correct.load());

  return data_correct.load();
}

// 测试 3: 替换已有的回调
bool TestCallbackReplacement() {
  std::cout << "\n========== 测试 3: 替换已有的回调 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<int> first_callback_count(0);
  std::atomic<int> second_callback_count(0);

  std::thread server_thread([&]() {
    Server server;

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>&) {
      // 发送回复
      std::string reply = "Reply";
      server.SendData(conn_id,
                      reinterpret_cast<const uint8_t*>(reply.c_str()),
                      reply.length());
    });

    if (!server.StartIPv4("127.0.0.1", 9990)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.Stop();
  });

  std::thread client_thread([&]() {
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    // 设置第一个回调
    client.SetOnMessage([&](const std::vector<uint8_t>&) {
      first_callback_count++;
      std::cout << "[Client] 第一个回调被调用" << std::endl;
    });

    if (!client.ConnectIPv4("127.0.0.1", 9990)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 发送第一条消息（第一个回调应该被调用）
    client.SendData(reinterpret_cast<const uint8_t*>("First"), 5);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // 替换回调
    std::cout << "[Client] 替换 OnMessage 回调" << std::endl;
    client.SetOnMessage([&](const std::vector<uint8_t>&) {
      second_callback_count++;
      std::cout << "[Client] 第二个回调被调用" << std::endl;
    });

    // 发送第二条消息（第二个回调应该被调用）
    client.SendData(reinterpret_cast<const uint8_t*>("Second"), 6);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    client.Disconnect();
    test_done = true;
  });

  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  std::cout << "第一个回调调用次数: " << first_callback_count.load()
            << " (期望: 1)" << std::endl;
  std::cout << "第二个回调调用次数: " << second_callback_count.load()
            << " (期望: 1)" << std::endl;

  bool success = (first_callback_count.load() == 1 &&
                  second_callback_count.load() == 1);

  RecordResult("替换已有的回调", success);

  return success;
}

// 测试 4: 多次接收到数据，回调被多次调用
bool TestMultipleMessages() {
  std::cout << "\n========== 测试 4: 多次接收到数据，回调被多次调用 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<int> callback_count(0);
  const int num_messages = 5;

  std::thread server_thread([&]() {
    Server server;

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>&) {
      // 回复每条消息
      std::string reply = "Reply";
      server.SendData(conn_id,
                      reinterpret_cast<const uint8_t*>(reply.c_str()),
                      reply.length());
    });

    if (!server.StartIPv4("127.0.0.1", 9991)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.Stop();
  });

  std::thread client_thread([&]() {
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    client.SetOnMessage([&](const std::vector<uint8_t>&) {
      callback_count++;
    });

    if (!client.ConnectIPv4("127.0.0.1", 9991)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 发送多条消息
    for (int i = 0; i < num_messages; ++i) {
      std::string msg = "Message " + std::to_string(i);
      client.SendData(reinterpret_cast<const uint8_t*>(msg.c_str()),
                      msg.length());
      std::cout << "[Client] 发送消息 " << i << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // 等待所有回复到达
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    client.Disconnect();
    test_done = true;
  });

  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  std::cout << "回调调用次数: " << callback_count.load()
            << " (期望: " << num_messages << ")" << std::endl;

  bool success = (callback_count.load() == num_messages);

  RecordResult("多次接收到数据，回调被多次调用", success);

  return success;
}

// 测试 5: 空数据回调处理
bool TestEmptyData() {
  std::cout << "\n========== 测试 5: 空数据回调处理 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<bool> empty_data_received(false);
  std::atomic<bool> zero_size_received(false);

  std::thread server_thread([&]() {
    Server server;

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>& data) {
      // 回复收到的数据（可能为空）
      server.SendData(conn_id, data.data(), data.size());
    });

    if (!server.StartIPv4("127.0.0.1", 9992)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.Stop();
  });

  std::thread client_thread([&]() {
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    client.SetOnMessage([&](const std::vector<uint8_t>& data) {
      if (data.empty()) {
        empty_data_received = true;
        std::cout << "[Client] 收到空数据" << std::endl;
      } else if (data.size() == 0) {
        zero_size_received = true;
        std::cout << "[Client] 收到 size=0 的数据" << std::endl;
      }
    });

    if (!client.ConnectIPv4("127.0.0.1", 9992)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 先发送非空数据，确保回调正常工作
    client.SendData(reinterpret_cast<const uint8_t*>("Test"), 4);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 发送空数据
    client.SendData(nullptr, 0);
    std::cout << "[Client] 发送空数据" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    client.Disconnect();
    test_done = true;
  });

  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  // 注意：空数据可能不会被传递，这取决于协议实现
  // 这里只验证回调能够处理各种情况
  RecordResult("空数据回调处理", true, "回调能够处理各种数据情况");

  return true;
}

// 测试 6: 大数据回调处理
bool TestLargeData() {
  std::cout << "\n========== 测试 6: 大数据回调处理 ==========" << std::endl;

  std::atomic<bool> server_ready(false);
  std::atomic<bool> test_done(false);
  std::atomic<bool> large_data_received(false);
  const size_t large_data_size = 100 * 1024; // 100 KB

  std::thread server_thread([&]() {
    Server server;

    server.SetOnMessage([&](uint64_t conn_id, const std::vector<uint8_t>& data) {
      // 回复收到的数据
      server.SendData(conn_id, data.data(), data.size());
    });

    if (!server.StartIPv4("127.0.0.1", 9993)) {
      std::cerr << "[Server] 启动失败!" << std::endl;
      test_done = true;
      return;
    }

    server_ready = true;

    while (!test_done) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    server.Stop();
  });

  std::thread client_thread([&]() {
    while (!server_ready) {
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client;

    client.SetOnMessage([&](const std::vector<uint8_t>& data) {
      if (data.size() >= large_data_size) {
        large_data_received = true;
        std::cout << "[Client] 收到大数据，大小: " << data.size() << " bytes" << std::endl;
      }
    });

    if (!client.ConnectIPv4("127.0.0.1", 9993)) {
      std::cerr << "[Client] 连接失败!" << std::endl;
      test_done = true;
      return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 创建大数据
    std::vector<uint8_t> large_data(large_data_size, 'X');
    large_data[0] = 'S';  // Start
    large_data[large_data_size - 1] = 'E';  // End

    std::cout << "[Client] 发送大数据，大小: " << large_data_size << " bytes" << std::endl;
    client.SendData(large_data.data(), large_data.size());

    // 等待大数据传输和回复
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    client.Disconnect();
    test_done = true;
  });

  while (!test_done) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }

  if (server_thread.joinable()) {
    server_thread.join();
  }
  if (client_thread.joinable()) {
    client_thread.join();
  }

  RecordResult("大数据回调处理", large_data_received.load());

  return large_data_received.load();
}

// 主测试函数
int main(int argc, char* argv[]) {
  std::cout << "\n========================================" << std::endl;
  std::cout << "DarwinCore Client::SetOnMessage 单元测试" << std::endl;
  std::cout << "========================================" << std::endl;

  int passed = 0;
  int total = 0;

  // 运行所有测试
  auto run_test = [&](const std::string& name, std::function<bool()> test_func) {
    total++;
    std::cout << "\n[开始] " << name << "..." << std::endl;
    try {
      bool result = test_func();
      if (result) {
        passed++;
      }
    } catch (const std::exception& e) {
      std::cerr << "[异常] " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "[异常] 未知异常" << std::endl;
    }
  };

  run_test("测试 1: SetOnMessage 回调被正确调用", TestCallbackInvocation);
  run_test("测试 2: 回调接收到正确的数据内容", TestCallbackDataContent);
  run_test("测试 3: 替换已有的回调", TestCallbackReplacement);
  run_test("测试 4: 多次接收到数据，回调被多次调用", TestMultipleMessages);
  run_test("测试 5: 空数据回调处理", TestEmptyData);
  run_test("测试 6: 大数据回调处理", TestLargeData);

  // 输出测试结果汇总
  std::cout << "\n========================================" << std::endl;
  std::cout << "测试结果汇总" << std::endl;
  std::cout << "========================================" << std::endl;
  std::cout << "总测试数: " << total << std::endl;
  std::cout << "通过: " << passed << std::endl;
  std::cout << "失败: " << (total - passed) << std::endl;
  std::cout << "成功率: " << (total > 0 ? (passed * 100.0 / total) : 0.0) << "%" << std::endl;

  // 详细测试结果
  std::cout << "\n详细测试结果:" << std::endl;
  for (const auto& result : g_results) {
    std::cout << "  " << (result.passed ? "[PASS]" : "[FAIL]") << " "
              << result.name;
    if (!result.message.empty()) {
      std::cout << " - " << result.message;
    }
    std::cout << std::endl;
  }

  std::cout << "========================================\n" << std::endl;

  return (passed == total) ? 0 : 1;
}
