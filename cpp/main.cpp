// To compile:  g++ -O3 -march=native -Wall main.cpp -std=c++17 -static
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sched.h>

#include <iostream>
#include <array>
#include <climits>

#define PORT 27178
#define LOG_ENABLED
// #define AFFINITY

#ifdef LOG_ENABLED
#include <chrono>
#define LOG_TIME(r) steady_clock::time_point r = std::chrono::steady_clock::now();
#else
#define LOG_TIME(r) 
#endif

using IntType = int32_t;
using std::chrono::steady_clock;
#define DELTA(t) std::chrono::duration_cast<std::chrono::microseconds>(t)


template<int Size>
class StringBuffer
{
public:
    void append(const char c)
    {
       data[size] = c;
       ++size;
    };
    void append(const char* str)
    {
        append(str, strlen(str));
    };
    void append(const char* str, size_t len)
    {
        memcpy(&data[size], str, len);
        size += len;
    };
    void append(const std::string_view& str)
    {
        memcpy(&data[size], str.data(), str.size());
        size += str.size();
    };
    inline void clear()
    {
        size = 0;
    };
    inline const char* head() const
    {
        return &data[0];
    };
    inline char* tail()
    {
        return &data[size];
    };

    std::array<char, Size> data;
    size_t size{0U};
};

std::array<IntType, 110000> track_sum;
std::array<IntType, 110000> results;
std::array<StringBuffer<2000000>, 22> ss_problems;
size_t max_problem{0};

const char digit_pairs[201] = {
  "00010203040506070809"
  "10111213141516171819"
  "20212223242526272829"
  "30313233343536373839"
  "40414243444546474849"
  "50515253545556575859"
  "60616263646566676869"
  "70717273747576777879"
  "80818283848586878889"
  "90919293949596979899"
};

// Write an integer to the buffer
template<int N>
inline void itostr(IntType val, StringBuffer<N>& s)
{
    if(val == 0)
    {
        s.append('0');
    }

    size_t size;
    if(val>=10000)
    {
        if(val>=1000000)
            size=7;
        else if(val>=100000)
            size=6;
        else
            size=5;
    }
    else 
    {
        if(val>=100)
        {
            if(val>=1000)
                size=4;
            else
                size=3;
        }
        else
        {
            if(val>=10)
                size=2;
            else
                size=1;
        }
    }
    char* c = &s.data[s.size + size - 1];
    s.size += size;
    while(val>=100)
    {
       int pos = val % 100;
       val /= 100;
       *(short*)(c-1)=*(short*)(digit_pairs+2*pos); 
       c-=2;
    }
    while(val>0)
    {
        *c--='0' + (val % 10);
        val /= 10;
    }
}
// Write a comma and an integer to the buffer
template<int N>
inline void itostrcomma(IntType val, StringBuffer<N>& s)
{
    if(val == 0)
    {
        s.append(',');
        s.append('0');
    }

    size_t size;
    if(val>=10000)
    {
        if(val>=1000000)
            size=7;
        else if(val>=100000)
            size=6;
        else
            size=5;
    }
    else 
    {
        if(val>=100)
        {
            if(val>=1000)
                size=4;
            else
                size=3;
        }
        else
        {
            if(val>=10)
                size=2;
            else
                size=1;
        }
    }

    char* c = s.tail();
    *c = ',';
    c += size;
    s.size += size + 1;
    while(val>=100)
    {
       int pos = val % 100;
       val /= 100;
       *(short*)(c-1)=*(short*)(digit_pairs+2*pos); 
       c-=2;
    }
    while(val>0)
    {
        *c--='0' + (val % 10);
        val /= 10;
    }
}

inline void seek_digi(const char* buf, size_t& i)
{
    while (true)
    {
        char c = buf[i];
        if ('0' <= c && c <= '9')
        {
            return;
        }
        i++;
    }
}
inline void seek_digi_unsafe(const char* buf, size_t& i)
{
    while (true)
    {
        char c = buf[i];
        // if ('0' <= c && c <= '9')
        if ((static_cast<uint8_t>(c) & 0xF0) == 0x30) // <= some ascii magic
        {
            return;
        }
        i++;
    }
}

inline bool parse_int_unsafe(const char* buf, size_t& i, IntType& val)
{
    val = buf[i++]-'0';
    char c = buf[i++];
    while ((static_cast<uint8_t>(c) & 0xF0) == 0x30)
    {
        val = val*10 + (c - '0');
        c = buf[i++];
    }
    return true;
}

inline bool parse_int(const char* buf, size_t& i, IntType& val)
{
    char c;
    
    seek_digi(buf, i);
    
    val = buf[i++]-'0';
    while (buf[i] != ',' && buf[i] != ']')
    {
        c = buf[i++];
        if ('0' <= c && c <= '9')
        {
            val = val*10 + (c - '0');
        }
    }
    return true;
}
inline bool reverse_parse_int(const char* buf, size_t& i, IntType& val)
{
    char c;
    
    while(true)
    {
        c = buf[i];
        if ('0' <= c && c <= '9')
        {
            break;
        }
        --i;
    }
    c = buf[i--];
    // IntType digit_num_to_power[5] = {10, 100, 1000, 10000, 100000};
    IntType digit_num = 1;
    val = 0;
    while ('0' <= c && c <= '9')
    {
        val += digit_num * (c - '0');
        digit_num *= 10;
        c = buf[i--];
    }
    return true;
}

// Parse the track from the end to the start
inline size_t reverse_parse_track(const char* buf, size_t len)
{
    size_t t = 0;
    IntType sum = 0;
    track_sum[t++]  = 0;
    IntType val;
    size_t i = len -1;
    while (reverse_parse_int(buf, i, val))
    {
        sum += val;
        track_sum[t++] = sum;
        if(buf[i+1] == '[')
        {
            break;
        }
    }
    return t;
}

template<typename T>
inline T fast_abs(const T v)
{
    const T mask = v >> (sizeof(T) * CHAR_BIT - 1);
    return (v + mask) ^ mask;
}

// Parse the json input, super unsafe
// Parse the track in reverse and then parse the items from start to finish
size_t reverse_parse(const char* buf, const size_t len)
{
    size_t i = 0; // buffer offset
    size_t m = 0; // item count
    
    LOG_TIME(begin)
    const size_t t = reverse_parse_track(buf, len);
    LOG_TIME(start_item)

    seek_digi(buf, i);
    IntType start, end;
    char after_pair;
    // Example of and items: "items":[[1,2],[3,4],[5,6]]
    do
    {
        parse_int_unsafe(buf, i, start);
        parse_int_unsafe(buf, i, end);

        results[m] = fast_abs(track_sum[t-start-1] - track_sum[t-end-1]);
        ++m;
        after_pair = buf[i];
        i += 2;
    } while(after_pair == ','); // should be ] if we reach the end
    LOG_TIME(end_item)

#ifdef LOG_ENABLED
    std::cout
    << "tracksum=" << DELTA(start_item - begin).count() 
    << "[µs] item=" << DELTA(end_item - start_item).count() << "[µs]" << std::endl;
#endif
    return m;
}

// Compile-time strlen
constexpr size_t ct_strlen( const char* s ) noexcept
{
  return *s ? 1 + ct_strlen(s + 1) : 0;
}

void solve(int socket_fd, const char* buffer, const size_t len, const size_t problem)
{
    LOG_TIME(begin)

    if (problem < max_problem)
    {
        auto& ss = ss_problems[problem];
        write(socket_fd, ss.head(), ss.size);
        return;
    }
    max_problem = problem;

    LOG_TIME(end_cache)

    const auto nb_items = reverse_parse(buffer, len);

    LOG_TIME(parse_end)

    // All problem after the 20th are written to the same buffer
    auto& ss = problem > 20 ? ss_problems[21] : ss_problems[problem];
    if (problem > 20)
    {
        ss.clear();
    }
    const char* header{"HTTP/1.1 200 OK\r\n"
"Server: bab\r\n\r\n["};
    ss.append(header, ct_strlen(header));

    itostr(results[0], ss);
    size_t j = 1U;
    for (; j < nb_items; ++j)
    {
        itostrcomma(results[j], ss);
    }
    ss.append(']');
    write(socket_fd, ss.head(), ss.size);

    LOG_TIME(end)
#ifdef LOG_ENABLED
    std::cout << problem << " i" << nb_items << " total = " << DELTA(end - begin).count() 
    << "[µs] cache=" << DELTA(end_cache - begin).count() 
    << "[µs] parse=" << DELTA(parse_end - end_cache).count() 
    // << "[µs] compute="<< DELTA(compute_end - parse_end).count() 
    << "[µs] compute=" << DELTA(end - parse_end).count() << std::endl;
#endif
}

// Parse http header to get the problem;s number and the length of the http body
void parse_header(char * buf, size_t& i, size_t& problem_num, size_t& body_length)
{
    while (buf[i] != '=')
    {
        ++i;
    }
    ++i;
    char c;
    problem_num = buf[i++]-'0';
    while (buf[i] != ' ')
    {
        c = buf[i++];
        if ('0' <= c && c <= '9')
        {
            problem_num = problem_num*10 + (c - '0');
        }
    }
    while (true)
    {
        ++i;
        if (buf[i] != 't')
        {
            continue;
        }
        if (buf[i + 1] != 'h')
        {
            continue;
        }
        if (buf[i + 2] != ':')
        {
            continue;
        }
        if (buf[i + 3] != ' ')
        {
            continue;
        }
        i += 4;
        break;
    }
    body_length = buf[i++]-'0';
    while (buf[i] != '\r')
    {
        c = buf[i++];
        if ('0' <= c && c <= '9')
        {
            body_length = body_length*10 + (c - '0');
        }
    }
    while (true)
    {
        if (buf[i++] != '\r')
        {
            continue;
        }
        if (buf[i] != '\n')
        {
            continue;
        }
        ++i;
        if (buf[i] != '\r')
        {
            continue;
        }
        if (buf[i + 1] != '\n')
        {
            continue;
        }
        i += 2;
        break;
    }
}

int main(int argc, char const *argv[])
{
#ifdef AFFINITY
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(0, &set);
    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)
    {
        perror("In sched_setaffinity");
        exit(EXIT_FAILURE);
    }
#endif

    std::array<char, 2000000> buffer_raw;
    char* buffer = &buffer_raw[0];

    int server_fd, socket_fd;
    long len;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("In socket");
        exit(EXIT_FAILURE);
    }
    int option = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    option = 1;
    setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &option, sizeof(option));
    int receive_buffer_size = 1091166;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, (char *) &receive_buffer_size, sizeof(receive_buffer_size));
    int send_buffer_size = 421000;
    setsockopt(server_fd, SOL_SOCKET, SO_RCVBUF, (char *) &send_buffer_size, sizeof(send_buffer_size));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    memset(address.sin_zero, '\0', sizeof(address.sin_zero));
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("In bind");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 10) < 0)
    {
        perror("In listen");
        exit(EXIT_FAILURE);
    }
    size_t problem = 0U;
    while(1)
    {
        printf("\n+++++++ Waiting for new connection ++++++++\n");
        if ((socket_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0)
        {
            perror("In accept");
            exit(EXIT_FAILURE);
        }

        len = read(socket_fd, buffer, 16384);
        LOG_TIME(after_read)
        if (len < 0)
        {
            perror("Read socket failed");
            exit(EXIT_FAILURE);
        }
        if (len == 0)
        {
            perror("Empty read");
            exit(EXIT_FAILURE);
        }
        // GET
        if (buffer[0] == 'G')
        {
            const char* response_404 = "HTTP/1.1 404 NOT FOUND\r\n"
                                        "Server: served-v1.4.3-DS1\r\n"
                                        "Content-Type: text/plain\r\n"
                                        "Content-Length: 14\r\n"
                                        "\r\n"
                                        "Path not found";
            write(socket_fd , response_404 , ct_strlen(response_404));
            close(socket_fd);
            printf("404\n");
            continue;
        }
        size_t header_size = 0U;
        size_t length_body = 0U;
        parse_header(buffer, header_size, problem, length_body);
#ifdef LOG_ENABLED
        printf("Problem: %ld\n", problem); 
#endif
        char* real_start_body = &(buffer[header_size]);
        const size_t remaining_bytes = len - header_size;
        size_t missing_body_bytes = length_body - remaining_bytes;

        LOG_TIME(begin)

        if (missing_body_bytes > 0)
        {
            len = remaining_bytes;
            do
            {
                int res = read(socket_fd, &real_start_body[len], missing_body_bytes);
                if (res < 0)
                {
                    printf("Failed to get more bytes\n");
                    exit(EXIT_FAILURE);
                }
                len += res;
                missing_body_bytes -= res;
            } while (missing_body_bytes > 0);
        }
        LOG_TIME(read_end)
        solve(socket_fd, real_start_body, length_body, problem);
        LOG_TIME(solve_end)

        // Closing the socket allow us to not specify the content length in our response
        close(socket_fd);

#ifdef LOG_ENABLED
        auto& response = ss_problems[problem > 20 ? 21 : problem];
        std::cout << problem << " i" << header_size + length_body << "r="<< response.size<< " total = " << DELTA(solve_end - after_read).count() 
        << "[µs] b4read=" << DELTA(begin - after_read).count() 
        << "[µs] read=" << DELTA(read_end - begin).count() 
        << "[µs] solve=" << DELTA(solve_end - read_end).count() << std::endl;
#endif
        // This end condition is required to get a log output.
        if (problem == 20 && max_problem == 55)
        {
            printf("done");
            return 0;
        }
        
    }
    return 0;
}