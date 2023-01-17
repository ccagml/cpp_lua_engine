/*
 * Filename: https://github.com/ccagml/stunning/src/config_manager.cpp
 * Path: https://github.com/ccagml/stunning/src
 * Created Date: Tuesday, January 17th 2023, 7:42:05 pm
 * Author: ccagml
 *
 * Copyright (c) 2023 ccagml . All rights reserved
 */

#include "config_manager.hpp"

namespace stunning {

ConfigManager::ConfigManager() {}

ConfigManager *ConfigManager::GetInstance() {
  static ConfigManager _ml_obj;
  return &_ml_obj;
}

// 获取vs类型配置
std::vector<std::string> ConfigManager::getConfigVS(std::string &key) {
  return config_vs[key];
}
std::string ConfigManager::getConfigS(std::string &key) {
  return all_config[key];
}

bool ConfigManager::readJsonFile(std::string &config_path, std::string &key) {
  // std::ifstream jsonFile(config_path);

  boost::property_tree::ptree pt;
  boost::property_tree::read_json(config_path, pt);

  for (auto &config_obj : pt) {
    std::string SERVER_NAME = config_obj.second.get<std::string>("SERVER_NAME");
    if (SERVER_NAME.compare(key) == 0) {
      for (auto &property : config_obj.second) {
        if (property.first.compare("db") == 0) {
          for (auto &db_info : property.second) {
            std::string db_name = db_info.second.get<std::string>("db_name");
            std::string password = db_info.second.get<std::string>("password");
            std::string host = db_info.second.get<std::string>("host");
            std::string username = db_info.second.get<std::string>("username");
            std::string port = db_info.second.get<std::string>("port");
            std::shared_ptr<stunning::ConfigDbInfo> db_shared_obj;
            if (db_config.count(db_name) > 0) {
              db_shared_obj = db_config[db_name];
            } else {
              db_shared_obj = std::make_shared<stunning::ConfigDbInfo>();
            }
            db_shared_obj->set_db_info("db_name", db_name);
            db_shared_obj->set_db_info("password", password);
            db_shared_obj->set_db_info("host", host);
            db_shared_obj->set_db_info("username", username);
            db_shared_obj->set_db_info("port", port);
            db_config[db_name] = db_shared_obj;
          }
        } else if (property.first.compare("listen") == 0) {
          std::vector<std::string> vec = {};
          BOOST_FOREACH (boost::property_tree::ptree::value_type &v,
                         property.second) {
            std::cout << v.second.data() << std::endl;
            vec.push_back(v.second.data());
          }
          config_vs["listen"] = vec;
        } else {
          all_config[property.first] = property.second.get_value<std::string>();
        }
      }
    }
  }
  return false;
}

ConfigManager::~ConfigManager() {}

RetCode ConfigManager::Init(std::string &config_path, std::string &key) {
  all_config.clear();
  db_config.clear();
  this->readRootPath();
  this->readJsonFile(config_path, key);
  return NO_ERROR;
}

std::shared_ptr<ConfigDbInfo> ConfigManager::getDbConfig(std::string &db_name) {
  return db_config[db_name];
}

void ConfigManager::readRootPath() {
  boost::filesystem::path full_path(boost::filesystem::current_path());
  std::cout << "Current path is : " << full_path << std::endl;
  std::cout << "Current path is : " << full_path << std::endl;
  all_config["root_path"] = full_path.string() + "/";
}

}  // namespace stunning
