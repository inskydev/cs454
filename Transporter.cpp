#include <string>
using namespace std;

struct Transporter {
  Transporter(string hostname, int port)
    : m_hostname(hostname),
      m_port(port)
  {
  }

  void connect();

  // Client only.
  string query();

  // Server only
  string reply();

  void disconnect();

  string m_hostname;
  int m_port;
};
