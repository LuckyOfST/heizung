#include "MQTTClient.h"

#ifdef SUPPORT_MQTT

#include <MQTT.h>

#include "Network.h"
#include "Tools.h"

extern void interpret( Stream& stream, Stream& out );

namespace MQTT{

  namespace{
    
    const char* mqttClientId = "heizung";
    const char* mqttBrokerIP = "nas";
    const int mqttBrokerPort = 1883;

    EthernetClient mqttNetClient;
    MQTTClient mqttClient;

  } // namespace

  bool is( const char* head, const String& topic, String& name ){
    if ( !topic.startsWith( head ) ){
      return false;
    }
    name = topic.substring( strlen( head ) );
    name.replace( '/', '_' );
    return true;
  }

  void onMessageReceived( String& topic, String& msg ){
    if ( !msg.length() ){
      return;
    }
    mqttClient.publish( "/heizung/cmd", topic + ": " + msg );
    String cmd;
    String name;
    if ( is( "/heizung/set/temp/", topic, name ) ){
      cmd = "set " + name + ' ' + msg;
    } else if ( is( "/heizung/set/switch/", topic, name ) ){
      cmd = "forceController " + name + ' ' + ( msg == "0" ? "-1" : "1" );
    } else if ( is( "/heizung/set/min-level/", topic, name ) ){
      cmd = "setMinLevel " + name + ' ' + msg;
    } else if ( is( "/heizung/set/temp-profile/", topic, name ) ){
      cmd = "setProfile " + name + ' ' + msg;
    }
    if ( cmd.length() > 0 && name.length() > 0 ){
      mqttClient.publish( topic.c_str(), "", true, 0 ); // force 'set' messages to be not retained!
      StringStream in( cmd );
      StringStream out;
      interpret( in, out );
      mqttClient.publish( "/heizung/cmd", cmd + " -> " + out.str(), true, 1 );
    }
  }

  void setup(){
    mqttClient.begin( mqttBrokerIP, mqttBrokerPort, mqttNetClient );
    mqttClient.onMessage( &onMessageReceived );
    //mqttClient.setWill( "/heizung/state/running", "0", true, 1 );
    mqttClient.setOptions( 1000, true, 1000000 );
    loop();
    //mqttClient.publish( "/heizung/state/running", "1", true, 1 );
  }

  void loop(){
    if ( !mqttClient.connected() ){
      if ( mqttClient.connect( mqttClientId ) ){
        mqttClient.subscribe( "/heizung/set/#" );
      }
    }
    mqttClient.loop();
  }
  
  void publish( const String& category, const String& name, const String& payload ){
    String topic = "/heizung/" + category + '/' + name;
    topic.replace( '_', '/' );
    mqttClient.publish( topic, payload, true, 1 );
  }

  void publishTemp( const String& name, float t ){
    publish( "state/temp", name, String( t, 1 ) );
  }

  void publishTempSetup( const String& name, float t, float minLevel, int profile ){
    publish( "settings/temp", name, String( t, 1 ) );
    publish( "settings/min-level", name, String( minLevel ) );
    publish( "settings/temp-profile", name, String( profile ) );
  }

  void publishSwitch( const String& name, bool on ){
    publish( "state/switch", name, on ? "1" : "0" );
  }

} // namespace MQTT

#endif
