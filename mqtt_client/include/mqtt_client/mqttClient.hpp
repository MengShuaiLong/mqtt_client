/**
 * @file mqttClient.hpp
 * @author mengshuailong 
 * @version 0.1
 * @date 2023-07-16
 * 
 * @copyright Copyright (c) 2023  
 */
#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include "mqtt/async_client.h"
#include "listen.hpp"

class MqttClient
{
public:
	MqttClient(const std::string &server_id, const std::string &port, const std::string &client_id)
		: server_id_(server_id),
			port_(port),
			client_id_(client_id),
			cli_(new mqtt::async_client(std::string(server_id_ + ":" + port_), client_id_)),
			listen_(*(cli_.get()), connOpts_, std::bind(&MqttClient::RecMesage, this, std::placeholders::_1))
			{
				Init();
			};

	~MqttClient()
  {
    Disconnet();
  };

protected:
	void Init();
	void ConnetToServer();
	bool CreateWriter(const std::string& topic, std::vector<uint8_t> &msg);
	bool CreateReader(const std::string& topic);
  bool CancelReader(const std::string& topic);

	virtual void  RecMesage(mqtt::const_message_ptr &msg){}
	bool ConToMqttSeverIsOk();
	bool ConIsOk();
	bool Disconnet();
  void ClearTopic(const std::string &topic);

public:
	std::string server_id_;
	std::string port_;
	std::string client_id_;
	mqtt::connect_options connOpts_;
	std::shared_ptr<mqtt::async_client> cli_ = nullptr ;
	Listen listen_;
  std::vector<std::string> all_sub_topic;
  bool con_ok = false;
};
 
inline void MqttClient::Init()
{
  RCLCPP_INFO(rclcpp::get_logger("mqtt client"),"address:[%s], port:[%s]",server_id_.c_str(), port_.c_str());
  //配置客户端属性
  connOpts_.set_clean_session(true);
  connOpts_.set_user_name("admin");
  connOpts_.set_password("admin123456");
  connOpts_.set_connect_timeout(1);
  connOpts_.set_keep_alive_interval(3);
  connOpts_.set_automatic_reconnect(true);
  cli_->set_callback(listen_);
  all_sub_topic.clear();
}

inline void MqttClient::ConnetToServer()
{	
  try{
    RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "Waiting for the connection...");
    cli_->connect(connOpts_, nullptr, listen_)->wait_for(500);
  }
  catch (const mqtt::exception& e) {
    RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "unable to connect to mqtt server, reason[%s]", e.what());
  }
}

inline bool MqttClient::CreateWriter(const std::string& topic, std::vector<uint8_t> &msg)
{	
  try{
    bool ret = true;
    mqtt::delivery_token_ptr pubtok;
    if(ConIsOk()){
      pubtok = cli_->publish(topic, reinterpret_cast<char*>(msg.data()), msg.size(), QOS, false);
      pubtok->wait_for(500);
      ret = true;
    }else{
      ret = false;
    }
    return ret;
  }catch (const mqtt::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "create writer err , reason [%s]", e.what());
    return false;
  }
}

inline bool MqttClient::CreateReader(const std::string& topic)
{
  try
  {
    bool ret = true;
    if(ConIsOk()){
      cli_->subscribe(topic, QOS)->wait();
      if(std::find(all_sub_topic.begin(), all_sub_topic.end(), topic) == all_sub_topic.end()){
        all_sub_topic.emplace_back(topic);
      }
      ret = true;
    }else{
      ret = false;
    }
    return ret;
  }
  catch(const std::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "create reader err , reason [%s]", e.what());
    return false;
  }
}

inline bool MqttClient::CancelReader(const std::string &topic)
{
  try
  {
    bool ret = true;
    if(ConIsOk()){
      cli_->unsubscribe(topic)->wait();
      ClearTopic(topic);
      ret = true;
    }else{
      ret = false;
    }
    return ret;
  }
  catch(const std::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "camcel reader err , reason [%s]", e.what());
    return false;
  }
}

inline bool MqttClient::ConIsOk(){
  try
  {
    if(listen_.connect_.load()){
      if((!all_sub_topic.empty()) && (!con_ok)){
        for(auto &it : all_sub_topic){
          cli_->subscribe(it, QOS)->wait();
          RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "all_sub_topic_:[%s]", it.c_str());
        }
      }
      con_ok = true;
    }else{
      RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "connet again ...");
      ConnetToServer();
      con_ok = false;
    }
    return con_ok;
  }
  catch(const std::exception& e)
  {
    RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "con is err, reason [%s]", e.what());
    return false;
  }
}

inline bool MqttClient::Disconnet(){
	if(cli_->is_connected())
  {
    if((!all_sub_topic.empty())){
      for(auto &it : all_sub_topic){
        cli_->unsubscribe(it)->wait();
      }
    }
		cli_->disconnect()->wait();
		RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "disconnet ...");
	}
}

inline void MqttClient::ClearTopic(const std::string &topic){
  auto pos = std::find(all_sub_topic.begin(), all_sub_topic.end(), topic);
    if( pos != all_sub_topic.end()){
      all_sub_topic.erase(pos);
    }
  }
