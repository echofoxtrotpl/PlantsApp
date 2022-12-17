//
//  MqttViewModel.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 29/11/2022.
//

import Foundation
import CocoaMQTT

final class MQTTManager: ObservableObject {
    private var mqttClient: CocoaMQTT?
    private var identifier: String!
    private var host: String!
    private var topic: String!
    private var username: String!
    private var password: String!
    
    private var temperatureTopic = "seba123/temperature"
    private var humidityTopic = "seba123/humidity"
    
    @Published var receivedMessage: String = ""
    @Published var records: [String: RecordStruct] = [:]
    
    // MARK: Shared Instance
    private static let _shared = MQTTManager()

    // MARK: - Accessors
    class func shared() -> MQTTManager {
        return _shared
    }

    func initializeMQTT(host: String, identifier: String, username: String? = nil, password: String? = nil) {
        // If any previous instance exists then clean it
        if mqttClient != nil {
            mqttClient = nil
        }
        self.identifier = identifier
        self.host = host
        self.username = username
        self.password = password
        let clientID = "CocoaMQTT-\(identifier)-" + String(ProcessInfo().processIdentifier)

        // TODO: Guard
        mqttClient = CocoaMQTT(clientID: clientID, host: host, port: 1883)
        // If a server has username and password, pass it here
        if let finalusername = self.username, let finalpassword = self.password {
            mqttClient?.username = finalusername
            mqttClient?.password = finalpassword
        }
        mqttClient?.willMessage = CocoaMQTTMessage(topic: "will", string: "dieout")
        mqttClient?.keepAlive = 60
        mqttClient?.delegate = self
        mqttClient?.allowUntrustCACertificate = true
        mqttClient?.autoReconnect = true
    }

    func connect() {
        if let success = mqttClient?.connect(), success {
            print("connect")
        } else {
            print("disconnect")
        }
    }

    func subscribe(topic: String) {
        self.topic = topic
        mqttClient?.subscribe(topic, qos: .qos1)
    }

    func publish(with message: String, on topic: String) {
        mqttClient?.publish(topic, withString: message, qos: .qos1)
    }

    func disconnect() {
        mqttClient?.disconnect()
    }

    /// Unsubscribe from a topic
    func unSubscribe(topic: String) {
        mqttClient?.unsubscribe(topic)
    }

    /// Unsubscribe from a topic
    func unSubscribeFromCurrentTopic() {
        mqttClient?.unsubscribe(topic)
    }
    
    func currentHost() -> String? {
        return host
    }
}

extension MQTTManager: CocoaMQTTDelegate {
    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopics topics: [String]) {
        TRACE("topic: \(topics)")
    }
    
    func mqtt(_ mqtt: CocoaMQTT, didSubscribeTopics success: NSDictionary, failed: [String]) {
        TRACE("topic: \(success)")
    }

    func mqtt(_ mqtt: CocoaMQTT, didConnectAck ack: CocoaMQTTConnAck) {
        TRACE("ack: \(ack)")
        subscribe(topic: humidityTopic)
        subscribe(topic: temperatureTopic)
    }

    func mqtt(_ mqtt: CocoaMQTT, didPublishMessage message: CocoaMQTTMessage, id: UInt16) {
        TRACE("message: \(message.string!.description), id: \(id)")
    }

    func mqtt(_ mqtt: CocoaMQTT, didPublishAck id: UInt16) {
        TRACE("id: \(id)")
    }

    func mqtt(_ mqtt: CocoaMQTT, didReceiveMessage message: CocoaMQTTMessage, id: UInt16) {
        TRACE("message: \(message.string!.description), id: \(id)")
        
        let decodedMessage = try! JSONDecoder().decode(MqttMessage.self, from: Data(message.string!.description.utf8))
        
        DispatchQueue.main.async {
            if self.records[decodedMessage.sensorName] == nil {
                self.records[decodedMessage.sensorName] = RecordStruct(temperature: 0.0, humidity: 0, updatedAt: Date.now, sensorName: decodedMessage.sensorName)
            }
        
            if message.topic == self.humidityTopic {
                self.records[decodedMessage.sensorName]?.humidity = decodedMessage.humidity!
            } else if message.topic == self.temperatureTopic {
                self.records[decodedMessage.sensorName]?.temperature = decodedMessage.temperature!
            }
            self.records[decodedMessage.sensorName]?.updatedAt = Date.now
        }
    }

    func mqtt(_ mqtt: CocoaMQTT, didUnsubscribeTopic topic: String) {
        TRACE("topic: \(topic)")
    }

    func mqttDidPing(_ mqtt: CocoaMQTT) {
        TRACE()
    }

    func mqttDidReceivePong(_ mqtt: CocoaMQTT) {
        TRACE()
    }

    func mqttDidDisconnect(_ mqtt: CocoaMQTT, withError err: Error?) {
        TRACE("\(err.debugDescription)")
    }
}

extension MQTTManager {
    func TRACE(_ message: String = "", fun: String = #function) {
        let names = fun.components(separatedBy: ":")
        var prettyName: String
        if names.count == 1 {
            prettyName = names[0]
        } else {
            prettyName = names[1]
        }

        if fun == "mqttDidDisconnect(_:withError:)" {
            prettyName = "didDisconect"
        }

        print("[TRACE] [\(prettyName)]: \(message)")
    }
}
