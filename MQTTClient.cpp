#include "MQTTClient.h"

#ifdef SUPPORT_MQTT

#include <MQTT.h>

#include "Network.h"
#include "Tools.h"

extern void interpret( Stream& stream, Stream& out );
extern bool g_setupInProgress;

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
    if ( g_setupInProgress ){
      return;
    }
    String cmd;
    String name;
    if ( is( "/heizung/setup/temp/", topic, name ) ){
      cmd = "set " + name + ' ' + msg;
    } else if ( is ( "/heizung/setup/switch/", topic, name ) ){
      cmd = "forceController " + name + ' ' + ( msg == "0" ? "-1" : "1" );
    } else if ( is( "/heizung/setup/min-level/", topic, name ) ){
      cmd = "setMinLevel " + name + ' ' + msg;
    } else if ( is( "/heizung/setup/temp-profile/", topic, name ) ){
      cmd = "setProfile " + name + ' ' + msg;
    }
    if ( cmd.length() > 0 && name.length() > 0 ){
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
    //mqttClient.setOptions( 10, true, 10000 );
    loop();
    //mqttClient.publish( "/heizung/state/running", "1", true, 1 );
  }

  void loop(){
    if ( !mqttClient.connected() ){
      if ( mqttClient.connect( mqttClientId ) ){
        mqttClient.subscribe( "/heizung/setup/#" );
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
    StringStream payload;
    payload << _FLOAT( t, 1 );
    publish( "state/temp", name, payload.str() );
  }

  void publishTempSetup( const String& name, float t, float minLevel, int profile ){
    StringStream payload;
    payload << _FLOAT( t, 1 );
    publish( "setup/temp", name, payload.str() );
    payload.clear();
    payload << minLevel;
    publish( "setup/min-level", name, payload.str() );
    payload.clear();
    payload << profile;
    publish( "setup/temp-profile", name, payload.str() );
  }

  void publishSwitch( const String& name, bool on ){
    publish( "state/switch", name, on ? "1" : "0" );
  }

} // namespace MQTT

#endif
