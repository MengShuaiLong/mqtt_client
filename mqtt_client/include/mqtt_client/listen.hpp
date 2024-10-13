#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <cctype>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include "mqtt/async_client.h"
#include "rclcpp/rclcpp.hpp"

/**
 * 0：最多传递一次
 * 1：至少传递一次
 * 2：仅传递一次
 **/
const int	QOS = 1;

class action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	void on_failure(const mqtt::token& tok) override {
    try{
      std::cout << name_ << " failure";
      if (tok.get_message_id() != 0)
        std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
        std::cout << std::endl;

    }catch (const mqtt::exception& exc){
      std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
    }
	}

	void on_success(const mqtt::token& tok) override {
		std::cout << name_ << " success";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		auto top = tok.get_topics();
		if (top && !top->empty())
			std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
		std::cout << std::endl;
	}

public:
	action_listener(const std::string& name) : name_(name) {}
};

/////////////////////////////////////////////////////////////////////////////

/**
 * Local callback & listener class for use with the client connection.
 * This is primarily intended to receive messages, but it will also monitor
 * the connection to the broker. If the connection is lost, it will attempt
 * to restore the connection and re-subscribe to the topic.
 */
class Listen : public virtual mqtt::callback,
					public virtual mqtt::iaction_listener
{
	typedef std::function<void (mqtt::const_message_ptr &)> CallbackFunc;

	// The MQTT client
	mqtt::async_client& cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;

	void on_failure(const mqtt::token& tok) override {
    try{
      RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "connection attempt failed");
      connect_.store(false);
    }catch(const mqtt::exception& exc){
      RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "connection attempt failed--- %s", exc.what());
    }
	}
  
	void on_success(const mqtt::token& tok) override {}

	void connected(const std::string& cause) override {
		connect_.store(true);
    RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "mqtt con server ok");
	}

  void connection_lost(const std::string& cause) override {
    try{
      if (!cause.empty()){
          RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "cause:[%s]", cause.c_str());
        }
        RCLCPP_INFO(rclcpp::get_logger("mqtt client"), "con lost");
        connect_.store(false);
      }catch (const mqtt::exception& exc){
        RCLCPP_ERROR(rclcpp::get_logger("mqtt client"), "con lost--- %s", exc.what());
      }
	}

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		cb_(msg);
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {}

public:
	Listen(mqtt::async_client& cli, mqtt::connect_options& connOpts, const CallbackFunc &cb)
    : cli_(cli), 
      connOpts_(connOpts), 
      cb_(cb), 
      subListener_("Subscription"), 
      connect_(false)  {}
      
private:
	CallbackFunc cb_;

public:
	std::atomic_bool connect_ ;
};

