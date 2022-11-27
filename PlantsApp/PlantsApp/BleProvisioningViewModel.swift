//
//  BLEProvisioning.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 26/11/2022.
//

import Foundation
import ESPProvision

@MainActor class BleProvisioningViewModel: ObservableObject {
    private var provisionedDevice: ESPDevice?
    private var foundDevices: [ESPDevice] = []
    private var proofOfPossession: String = "abcd1234"
    @Published var foundDevicesNames: [String] = []
    @Published var connected = false
    @Published var wifiSettingsResponse = ""
    @Published var basicSettingsResponse = ""
    @Published var deviceId = ""
    
    init() {
        scanForDevices()
    }
    
    func provisionDevice(ssid: String, passphrase: String) {
        if let device = self.provisionedDevice {
            device.provision(ssid: ssid, passPhrase: passphrase) { status in
                DispatchQueue.main.async {
                    switch status {
                    case .success:
                        self.wifiSettingsResponse = "Konfiguracja wifi pomyślna"
                        break
                    case .configApplied:
                        break
                    default:
                        self.wifiSettingsResponse = "Wystąpił problem"
                        break
                    }
                }
            }
        }
    }
    
    func scanForDevices() {
        self.foundDevices.removeAll()
        self.foundDevicesNames.removeAll()
        ESPProvisionManager.shared.searchESPDevices(devicePrefix:"", transport:.ble, security:.secure) { deviceList, _ in
            DispatchQueue.main.async {
                if let devices = deviceList {
                    self.foundDevices = devices
                    for device in devices {
                        self.foundDevicesNames.append(device.name)
                    }
                }
            }
        }
    }
    
    func handleConnectionStatus(status: ESPSessionStatus, device: ESPDevice) {
        DispatchQueue.main.async {
            switch status {
            case .connected:
                self.connected = true
                self.provisionedDevice = device
                self.deviceId = device.name.sha256
                break
            default:
                break
            }
        }
    }
    
    func sendBasicConfigToDevice(_ config: BasicConfiguration) {
        let jsonData = try? JSONEncoder().encode(config)
        if let data = jsonData {
            if let device = provisionedDevice {
                device.sendData(path: "custom-data", data: data) { res, _ in
                    DispatchQueue.main.async {
                        self.basicSettingsResponse = String(data: res!, encoding: .utf8)!
                    }
                }
            }
        }
    }
}

extension BleProvisioningViewModel: ESPDeviceConnectionDelegate {
    func getProofOfPossesion(forDevice: ESPProvision.ESPDevice, completionHandler: @escaping (String) -> Void) {
        completionHandler(proofOfPossession)
    }
    
    func getUsername(forDevice: ESPProvision.ESPDevice, completionHandler: @escaping (String?) -> Void) {
        print("Get username")
    }
    
    func connectToDevice(_ deviceName: String) {
        for device in foundDevices {
            if device.name == deviceName {
                device.connect(delegate: self) { status in
                    self.handleConnectionStatus(status: status, device: device)
                }
            }
        }
    }
    
    func disconnect() {
        self.provisionedDevice?.disconnect()
        self.connected = false
        self.provisionedDevice = nil
    }
}

