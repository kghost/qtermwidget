#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <assert.h>  
#include <type_traits>
#include <numeric>
#include <tuple>
#include <vector>

namespace Pty {
	struct Message {
		static constexpr int TYPE_DATA = 0;
		static constexpr int TYPE_EXEC = 1;
		static constexpr int TYPE_WINCH = 2;
		static constexpr int TYPE_EXIT = 3;

		static constexpr int FLAG_SYMMETRIC_MESSAGE = 1 << 0; // this type of message if symmetric, a response is required from other end with same xid
		static constexpr int FLAG_CONTINUE = 1 << 1;

		uint8_t type;
#if __BYTE_ORDER == __LITTLE_ENDIAN
		uint8_t xid : 6;
		uint8_t flags : 2;
#elif __BYTE_ORDER == __BIG_ENDIAN
		uint8_t flags : 2;
		uint8_t xid : 6;
#else
# error "Please fix <bits/endian.h>"
#endif
		uint16_t length;
	};

	static_assert(std::is_pod<Message>::value);
	static_assert(sizeof(Message) == 4);

	class String {
		public:
			String() {}

			template<typename T>
			String(T x) : s(x) {}

			std::size_t space() {
				return sizeof(uint32_t) + s.length();
			}

			char* marshal(char* start, char* end) {
				assert(end - start >= space());
				*(uint32_t*)start = s.length();
				std::memcpy(start + sizeof(uint32_t), s.c_str(), s.length());
				return start + sizeof(uint32_t) + s.length();
			}

			const char* unmarshal(const char* start, const char* end) {
				if (end - start < sizeof(uint32_t)) throw std::runtime_error("not enough space");
				uint32_t len = *(uint32_t*)start;
				auto a = start + sizeof(len);
				if (end - a < len) throw std::runtime_error("not enough space");
				s = std::string(a, len);
				return a + len;
			}
		public:
			std::string s;
	};

	class Exec {
		public:
			std::size_t space() {
				return sizeof(uint32_t)*2 + prog.space() + std::accumulate(args.begin(), args.end(), 0, [](auto s, auto& e) {
					return s + e.space();
				}) + std::accumulate(envs.begin(), envs.end(), 0, [](auto s, auto& e) {
					return s + std::get<0>(e).space() + std::get<1>(e).space();
				});
			}

			char* marshal(char* start, char* end) {
				assert(end - start >= space());
				start = prog.marshal(start, end);

				assert(end - start >= sizeof(uint32_t));
				*(uint32_t*)start = args.size();
				start += sizeof(uint32_t);
				start = std::accumulate(args.begin(), args.end(), start, [end](auto start, auto& e) {
					return e.marshal(start, end);
				});

				assert(end - start >= sizeof(uint32_t));
				*(uint32_t*)start = envs.size();
				start += sizeof(uint32_t);
				return std::accumulate(envs.begin(), envs.end(), start, [end](auto start, auto& e) {
					start = std::get<0>(e).marshal(start, end);
					start = std::get<1>(e).marshal(start, end);
					return start;
				});
			}

			const char* unmarshal(const char* start, const char* end) {
				start = prog.unmarshal(start, end);

				if (end - start < sizeof(uint32_t)) throw std::runtime_error("not enough space");
				uint32_t len = *(uint32_t*)start;
				start += sizeof(uint32_t);
				args.resize(len);
				start = std::accumulate(args.begin(), args.end(), start, [end](auto start, auto& e) {
					return e.unmarshal(start, end);
				});

				if (end - start < sizeof(uint32_t)) throw std::runtime_error("not enough space");
				uint32_t envs_len = *(uint32_t*)start;
				start += sizeof(uint32_t);
				envs.resize(envs_len);
				return std::accumulate(envs.begin(), envs.end(), start, [end](auto start, auto& e) {
					start = std::get<0>(e).unmarshal(start, end);
					start = std::get<1>(e).unmarshal(start, end);
					return start;
				});
			}
		public:
			String prog;
			std::vector<String> args;
			std::vector<std::tuple<String, String>> envs;
	};

	class WinSize {
		public:
			std::size_t space() { return sizeof(*this); }

			char* marshal(char* start, char* end) {
				assert(end - start >= space());
				std::memcpy(start, this, sizeof(this));
				return start + sizeof(this);
			}

			const char* unmarshal(const char* start, const char* end) {
				if (end - start < space()) throw std::runtime_error("not enough space");
				std::memcpy(this, start, sizeof(this));
				return start + sizeof(this);
			}
		public:
			uint32_t cols;
			uint32_t rows;
	};
	static_assert(std::is_pod<WinSize>::value);

#pragma pack(push, 1)
	class Exit {
		public:
			std::size_t space() { return sizeof(*this); }

			char* marshal(char* start, char* end) {
				assert(end - start >= space());
				std::memcpy(start, this, sizeof(this));
				return start + sizeof(this);
			}

			const char* unmarshal(const char* start, const char* end) {
				if (end - start < space()) throw std::runtime_error("not enough space");
				std::memcpy(this, start, sizeof(this));
				return start + sizeof(this);
			}
		public:
			bool exited; // normal exit
			bool signaled;
			uint32_t exit_status;
			uint32_t terminate_signal;
	};
	static_assert(std::is_pod<Exit>::value);
#pragma pack(pop)
}

#endif // PROTOCOL_H
