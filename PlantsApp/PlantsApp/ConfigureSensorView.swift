//
//  ConfigureSensorView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 26/11/2022.
//

import SwiftUI

struct ConfigureSensorView: View {
    @State private var wifiSsid = ""
    @State private var wifiPassword = ""
    @State private var sensorFamiliarName = ""
    
    @ObservedObject public var bleProvisioningViewModel: BleProvisioningViewModel
    
    var body: some View {
        Form {
            Section("Nadaj nazwę"){
                TextField("Przyjazna nazwa", text: $sensorFamiliarName)
                Button("Wyślij") {
                    bleProvisioningViewModel.provisionDevice(ssid: wifiSsid, passphrase: wifiPassword)
                }
            }
            
            Section {
                
                TextField("SSID", text: $wifiSsid)
                TextField("Hasło", text: $wifiPassword)
                
                Button("Sprawdź połączenie") {
                    bleProvisioningViewModel.provisionDevice(ssid: wifiSsid, passphrase: wifiPassword)
                }
            } header: {
                HStack {
                    Text("Konfiguracja WiFi")
                    Spacer()
                    if !bleProvisioningViewModel.wifiProvisioned {
                        Image(systemName: "x.circle")
                            .foregroundColor(.red)
                    } else {
                        Image(systemName: "checkmark.circle")
                            .foregroundColor(.green)
                    }
                    
                }
            }
        }
        .navigationTitle("Konfiguracja czujnika")
        .navigationBarTitleDisplayMode(.inline)
    }
}

struct ConfigureSensorView_Previews: PreviewProvider {
    static var previews: some View {
        ConfigureSensorView(bleProvisioningViewModel: BleProvisioningViewModel())
    }
}
