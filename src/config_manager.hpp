#ifndef _STUNNING_CONFIG_MANAGER_H_
#define _STUNNING_CONFIG_MANAGER_H_

#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <cassert>
#include <exception>
#include <iostream>
#include <sstream>
#include <string>
// root_path
#include <boost/filesystem.hpp>
#include <memory>

#include "global_define.hpp"
#include "unordered_map"
#include "vector"

namespace stunning {

class ConfigDbInfo {
 public:
  std::string db_name;
  std::string host;
  std::string username;
  unsigned int port;
  std::string passwd;
  ConfigDbInfo(/* args */){};
  ~ConfigDbInfo(){};

  void set_db_info(const std::string &db_field,
                   const std::string &db_field_value) {
    if (db_field.compare("db_name") == 0) {
      this->db_name = db_field_value;
    } else if (db_field.compare("host") == 0) {
      this->host = db_field_value;
    } else if (db_field.compare("username") == 0) {
      this->username = db_field_value;
    } else if (db_field.compare("port") == 0) {
      this->port = std::stoi(db_field_value);
      if (this->port < 1) {
        this->port = 3306;
      }
    } else if (db_field.compare("password") == 0) {
      this->passwd = db_field_value;
    }
  }
};

class ConfigManager {
 public:
  ConfigManager();
  ~ConfigManager();
  RetCode Init(std::string &config_path, std::string &key);
  void readRootPath();
  static ConfigManager *GetInstance();
  std::unordered_map<std::string, std::string> all_config;
  std::unordered_map<std::string, std::shared_ptr<stunning::ConfigDbInfo>>
      db_config;

  std::unordered_map<std::string, std::vector<std::string>> config_vs;
  bool readJsonFile(std::string &config_path, std::string &key);
  std::shared_ptr<ConfigDbInfo> getDbConfig(std::string &db_name);
  std::vector<std::string> getConfigVS(std::string &key);
  std::string getConfigS(std::string &key);
};

}  // namespace stunning

#endif