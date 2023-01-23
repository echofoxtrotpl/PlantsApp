//
//  BluetoothViewModel.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import Foundation
import CoreBluetooth
import ESPProvision

class BluetoothViewModel : NSObject, ObservableObject {
    private var centralManager: CBCentralManager?
    private var peripherals: [CBPeripheral] = []
    private var connectedPeripheral: CBPeripheral?
    private var centralQueue: DispatchQueue?
    private var inputCharacteristic: CBCharacteristic?
    private var notifyCharacteristic: CBCharacteristic?
    
    @Published var foundDevicesNames: [String] = []
    @Published var connected = false
    @Published var wifiSettingsResponse = ""
    @Published var wifiSettingsApplied = false
    
    override init() {
        super.init()
        self.centralManager = CBCentralManager(delegate: self, queue: .main)
    }
    
    func connectToPeripheral(_ peripheralName: String) {
        for peripheral in peripherals {
            if peripheral.name == peripheralName {
                centralManager?.stopScan()
                centralManager?.connect(peripheral)
            }
        }
    }
    
    func send(_ message: String) {
        if let peripheral = connectedPeripheral, let inputCharacteristic = inputCharacteristic {
            let valueString = (message as NSString).data(using: String.Encoding.utf8.rawValue)
            peripheral.writeValue(valueString!, for: inputCharacteristic, type: .withResponse)
        }
    }
    
    func disconnect() {
        if let connectedPeripheral = connectedPeripheral {
            centralManager?.cancelPeripheralConnection(connectedPeripheral)
            DispatchQueue.main.async {
                self.connectedPeripheral = nil
                self.connected = false
            }
        }
    }
    
    func provisionDevice(ssid: String, password: String) {
        if connectedPeripheral != nil {
            let wifiCredentials = WifiCredentials(ssid: ssid, password: password)
            let encoded = try! JSONEncoder().encode(wifiCredentials)
            send(String(decoding: encoded, as: UTF8.self))
        }
    }
}

extension BluetoothViewModel: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            self.centralManager?.scanForPeripherals(withServices: nil)
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if !peripherals.contains(peripheral) {
            self.peripherals.append(peripheral)
            DispatchQueue.main.async {
                if let name = peripheral.name {
                    self.foundDevicesNames.append(name)
                }
            }
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        peripheral.delegate = self
        peripheral.discoverServices(nil)
    }
}

extension BluetoothViewModel: CBPeripheralDelegate {
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        guard let services = peripheral.services else {
            return
        }
            
        for service in services {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        guard let characteristics = service.characteristics else {
            return
        }
            
        for characteristic in characteristics {
            if characteristic.properties.contains(.notify) {
                notifyCharacteristic = characteristic
                peripheral.setNotifyValue(true, for: characteristic)
            }
            if characteristic.properties.contains(.write){
                inputCharacteristic = characteristic
            }
        }
            
        DispatchQueue.main.async {
            self.connected = true
            self.connectedPeripheral = peripheral
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
            if characteristic.uuid == notifyCharacteristic, let data = characteristic.value {
                let response = String(data: data, encoding: String.Encoding.utf8)!
                DispatchQueue.main.async {
                    if(response == "success"){
                        self.wifiSettingsResponse = "Successfuly provisioned device!"
                        self.wifiSettingsApplied = true
                    } else if(response == "fail"){
                        self.wifiSettingsResponse = "Couldn't provision device. Error occured."
                        self.wifiSettingsApplied = false
                    }
                }
            }
        }
}
