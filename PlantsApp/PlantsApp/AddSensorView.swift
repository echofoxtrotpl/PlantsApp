//
//  AddSensorView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 12/11/2022.
//

import SwiftUI

struct AddSensorView: View {
    @State private var sensorFamiliarName = ""
    @State private var selectedSensorName = ""
    @ObservedObject private var bluetoothViewModel = BluetoothViewModel()
    
    var body: some View {
        Form {
            Section(header: Text("Nadaj nazwę")){
                TextField("Przyjazna nazwa", text: $sensorFamiliarName)
            }
            if bluetoothViewModel.peripherialNames.count > 0 {
                Picker("Wyierz czujnik", selection: $selectedSensorName) {
                    ForEach(bluetoothViewModel.peripherialNames, id: \.self) { peripherial in
                        Text(peripherial)
                    }
                }
                .pickerStyle(.inline)
            } else {
                Text("Wyszukiwanie urządzeń...")
            }
            
        }
        .navigationTitle("Dodaj czujnik")
        .navigationBarTitleDisplayMode(.inline)
    }
}

struct AddSensorView_Previews: PreviewProvider {
    static var previews: some View {
        AddSensorView()
    }
}
