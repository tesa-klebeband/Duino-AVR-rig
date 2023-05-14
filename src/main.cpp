#include <curl/curl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <cstring>
#include <openssl/sha.h>
#include <thread>
#include <ctime>

struct Config
{
    char *wallet_address;
    char *mining_key;
    char *rig_name;
    int hashrate;
    int hash_time;
    int num_avrs;
};

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

std::string get_https(const std::string &url)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK)
        {
            curl_easy_cleanup(curl);
            return response;
        }
        else
        {
            fprintf(stderr, "No response from server!\n");
            curl_easy_cleanup(curl);
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "Failed to initialize cURL!\n");
        exit(1);
    }
}

int check_success(std::string str)
{
    if (str.find("\"success\":true") != std::string::npos)
    {
        return 1;
    }
    return 0;
}

int check_mining_key(Config *config)
{
    std::string url = "https://server.duinocoin.com/mining_key?u=";
    url += config->wallet_address;
    url += "&k=";
    url += config->mining_key;
    std::string response = get_https(url);
    return check_success(response);
}

int check_wallet(Config *config)
{
    std::string url = "https://server.duinocoin.com/users/";
    url += config->wallet_address;
    std::string response = get_https(url);
    return check_success(response);
}

int setup_connection(sockaddr_in *node_addr, int *sock, std::string *node_name)
{
    memset(node_addr, 0, sizeof(*node_addr));
    node_addr->sin_family = AF_INET;

    std::string response = get_https("https://server.duinocoin.com/getPool");
    if (!check_success(response))
        return 0;

    std::string temp = response.substr(response.find("\"ip\":\"") + 6);
    temp = temp.substr(0, temp.find("\","));
    node_addr->sin_addr.s_addr = inet_addr(temp.c_str());
    temp = response.substr(response.find("\"name\":\"") + 8);
    *node_name = temp.substr(0, temp.find("\","));
    temp = response.substr(response.find("\"port\":") + 7);
    temp = temp.substr(0, temp.find(","));
    node_addr->sin_port = htons(atoi(temp.c_str()));

    if (connect(*sock, (struct sockaddr *)node_addr, sizeof(*node_addr)) < 0)
    {
        return 0;
    }

    return 1;
}

uint16_t ducos1a(char *lastblockhash, char *newblockhash, uint16_t difficulty, int hash_time)
{
    newblockhash[40] = 0;
    lastblockhash[40] = 0;

    for (int i = 0; i < 40; i++)
    {
        if (newblockhash[i] >= 'a' && newblockhash[i] <= 'f')
        {
            newblockhash[i] -= 'a';
            newblockhash[i] += 'A';
        }
        i++;
    }

    uint8_t final_len = 40 >> 1;
    uint8_t job[104];
    for (uint8_t i = 0, j = 0; j < final_len; i += 2, j++)
        job[j] = ((((newblockhash[i] & 0x1F) + 9) % 25) << 4) + ((newblockhash[i + 1] & 0x1F) + 9) % 25;

    for (uint16_t ducos1res = 0; ducos1res < difficulty * 100 + 1; ducos1res++)
    {
        uint8_t hash_bytes[20];
        char str[46] = {0};
        int dsize = sprintf(str, "%s%d", lastblockhash, ducos1res);
        SHA1((unsigned char *)str, dsize, hash_bytes);
        if (memcmp(hash_bytes, job, 20) == 0)
            return ducos1res;
        usleep(hash_time * 1000);
    }
    return 0;
}

void mine_avr(Config *config, int avr_num)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in node_addr;
    std::string node_name;
    int num_shares = 0;
    int num_good_shares = 0;
    char DUCOID[23];

    while(!setup_connection(&node_addr, &sock, &node_name))
    {
        fprintf(stderr, "AVR-%03d: Unable to setup mining node! Retrying in 5s\n", avr_num);
        usleep(5000000);
    }

    fprintf(stderr, "AVR-%03d: Connected to %s!\n", avr_num, node_name.c_str());

    char version[6];
    char motd[1024];
    recv(sock, version, 6, 0);

    send(sock, "MOTD", 4, 0);
    recv(sock, motd, 1024, 0);

    fprintf(stderr, "AVR-%03d: Server message of the day: %s\n", avr_num, motd);

    std::srand(std::time(nullptr));
    int uuid1 = std::rand();
    std::srand(std::time(nullptr) * 2);
    int uuid2 = std::rand();
    sprintf(DUCOID, "DUCOID%08X%08X", uuid1, uuid2);

    while (1)
    {

        std::string job = "JOB,";
        job += config->wallet_address;
        job += ",AVR,";
        job += config->mining_key;
        send(sock, job.c_str(), job.length(), 0);

        char buffer[128];
        recv(sock, buffer, 128, 0);

        int diff = atoi(buffer + 82);

        int res = ducos1a(buffer, buffer + 41, diff, config->hash_time);
        num_shares++;

        job = std::to_string(res);
        job += ',';
        job += std::to_string(config->hashrate);
        job += ",Official AVR Miner 3.5,";
        job += config->rig_name;
        job += ',';
        job += DUCOID;
        send(sock, job.c_str(), job.length(), 0);

        char feedback[64];
        recv(sock, feedback, 64, 0);

        fprintf(stderr, "AVR-%03d: ", avr_num);

        if (strcmp(feedback, "GOOD") != 0)
        {
            num_good_shares++;
            fprintf(stderr, "✔ Accepted");
        } else {
            fprintf(stderr, "x Rejected");
        }

        fprintf(stderr, " %d/%d ⚙ %d H/s diff %d ⚙ Total Hashing power: %.1f kH/s\n", num_good_shares, num_shares, config->hashrate, diff, (float) (config->hashrate * config->num_avrs) / 1000.0);
    }
}

int main(int argc, char **argv)
{
    if (argc < 6)
    {
        fprintf(stderr, "Usage: duino-avr-rig [WALLET_ADDR] [MINING_KEY] [HASHRATE] [RIG_NAME] [NUM_AVRS]");
        exit(0);
    }

    Config config;
    config.wallet_address = argv[1];
    config.mining_key = argv[2];
    config.hashrate = atoi(argv[3]);
    config.rig_name = argv[4];
    config.hash_time = 1000 / config.hashrate;
    config.num_avrs = atoi(argv[5]);
    if (!check_wallet(&config))
    {
        fprintf(stderr, "Invalid wallet address!\n");
        exit(1);
    }

    if (!check_mining_key(&config))
    {
        fprintf(stderr, "Invalid mining key!\n");
        exit(1);
    }

    std::thread miners[config.num_avrs];
    for (int i = 0; i < config.num_avrs; i++)
    {
        miners[i] = std::thread(mine_avr, &config, i + 1);
        miners[i].detach();
    }

    while(1);
}