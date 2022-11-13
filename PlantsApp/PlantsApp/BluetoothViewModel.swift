//
//  BluetoothViewModel.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import Foundation
import CoreBluetooth

class BluetoothViewModel : NSObject, ObservableObject {
    private var centralManager: CBCentralManager?
    private var peripherials: [CBPeripheral] = []
    @Published var peripherialNames: [String] = []
    
    override init() {
        super.init()
        self.centralManager = CBCentralManager(delegate: self, queue: .main)
    }
}

extension BluetoothViewModel: CBCentralManagerDelegate {
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        if central.state == .poweredOn {
            self.centralManager?.scanForPeripherals(withServices: nil)
        }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        if !peripherials.contains(peripheral) {
            self.peripherials.append(peripheral)
            self.peripherialNames.append(peripheral.name ?? "unknown device")
        }
    }
}
