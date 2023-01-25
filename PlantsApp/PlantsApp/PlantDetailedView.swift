//
//  PlantDetailedView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct PlantDetailedView: View {
    var plant: Plant
    @AppStorage private var lastRecord: CodableWrapper<RecordStruct>
    @ObservedObject var mqttManager: MQTTManager
    @ObservedObject var httpClient: HttpClient
    @State private var editingHumidity = false
    @State private var editingTemperature = false
    @State private var maxHumidity: Int
    @State private var minHumidity: Int
    @State private var maxTemperature: Double
    @State private var minTemperature: Double
    let timer = Timer.publish(every: 5, on: .main, in: .common).autoconnect()
    
    init(mqttManager: MQTTManager, plant: Plant, httpClient: HttpClient) {
        self.plant = plant
        self._lastRecord = AppStorage(wrappedValue: .init(value: RecordStruct(temperature: 0.0, humidity: 0, updatedAt: Date(), sensorName: plant.sensorName)), plant.sensorName)
        self.mqttManager = mqttManager
        self.httpClient = httpClient
        self.maxHumidity = plant.maxHumidity
        self.minHumidity = plant.minHumidity
        self.maxTemperature = plant.maxTemperature
        self.minTemperature = plant.minTemperature
    }
    
    private var record: RecordStruct {
        get {
            mqttManager.records[plant.sensorName]?.last ?? lastRecord.value
        }
    }
    
    private var isTempExceeded: Bool {
        get {
            record.temperature > plant.maxTemperature ||
            record.temperature < plant.minTemperature
        }
    }
    
    var body: some View {
        ZStack{
            Color(red: 182, green: 224, blue: 189)
                .ignoresSafeArea()

            VStack{
                HStack{
                    ZStack{
                        Image("plants")
                            .resizable()
                            .blur(radius: 2)
                            .scaledToFit()
                        HStack{
                            VStack (alignment: .leading){
                                HStack{
                                    Text("Data ostatniej aktualizacji: ")
                                    Text(record.updatedAt, format: .dateTime)
                                }
                                .font(.caption)
                                Text(plant.familiarName)
                                    .font(.largeTitle)
                                    .bold()
                                Text(plant.location)
                            }
                            .shadow(radius: 3)
                            .padding(30)
                            Spacer()
                        }
                        .foregroundColor(.white)
                    }
                }
                .frame(height: 230)
                HStack{
                    ScrollView{
                        VStack(alignment: .leading, spacing: 30){
                            HStack{
                                Text("Aktualne warunki")
                                    .font(.title2)
                                    .bold()
                                    .padding(.bottom, 5)
                            }
                            HStack{
                                Text("Wilgotność")
                                Spacer()
                                ProgressChart(progress: Double(record.humidity)/100, maxValue: Double(plant.maxHumidity)/100, minValue: Double(plant.minHumidity)/100)
                                    .frame(width: 90, height: 90)
                            }
                            HStack{
                                Text("Temperatura")
                                Spacer()
                                if isTempExceeded {
                                    Image(systemName: "exclamationmark.triangle")
                                        .foregroundColor(.red)
                                }
                                Text(String(format: "%.1f°C", record.temperature))
                                    .foregroundColor(isTempExceeded ? .red : .black)
                            }
                            .animation(.easeInOut, value: record.temperature)
                        }
                        .padding(20)
                        VStack(alignment: .leading, spacing: 30){
                            HStack{
                                Text("Idealne warunki")
                                    .font(.title2)
                                    .bold()
                                    .padding(.bottom, 5)
                            }
                            HStack{
                                Button {
                                    editingHumidity = !editingHumidity
                                } label: {
                                    Image(systemName: "pencil")
                                }
                                Text("Wilgotność")
                                Spacer()
                                if !editingHumidity {
                                    Text("\(plant.minHumidity)% - \(plant.maxHumidity)%")
                                } else {
                                    HStack {
                                        Picker("Minimalna wilgotość", selection: $minHumidity) {
                                            ForEach(0...maxHumidity, id: \.self) {
                                                Text($0, format: .percent)
                                            }
                                        }
                                        Picker("Maksymalna wilgotość", selection: $maxHumidity) {
                                            ForEach(minHumidity...100, id: \.self) {
                                                Text($0, format: .percent)
                                            }
                                        }
                                    }
                                    
                                    Button("Zapisz"){
                                        Task {
                                            let res = await httpClient.updatePlant(Plant(id: plant.id, familiarName: plant.familiarName, location: plant.location, maxHumidity: maxHumidity, minHumidity: minHumidity, maxTemperature: plant.maxTemperature, minTemperature: plant.minTemperature, sensorName: plant.sensorName))
                                            if res {
                                                DispatchQueue.main.async {
                                                    editingHumidity = false
                                                }
                                                mqttManager.publish(with: "\(minHumidity)", on: "\(plant.sensorName)/minHumidity", retain: true)
                                                mqttManager.publish(with: "\(maxHumidity)", on: "\(plant.sensorName)/maxHumidity", retain: true)
                                            }
                                        }
                                    }
                                }
                            }
                            HStack{
                                Button {
                                    editingTemperature = !editingTemperature
                                } label: {
                                    Image(systemName: "pencil")
                                }
                                Text("Temperatura")
                                Spacer()
                                if !editingTemperature {
                                    Text("\(String(format: "%.1f°C", plant.minTemperature)) - \(String(format: "%.1f°C", plant.maxTemperature))")
                                } else {
                                    HStack {
                                        Picker("Minimalna temperatura", selection: $minTemperature) {
                                            ForEach(Array(stride(from: -20.0, to: maxTemperature, by: 0.1)), id: \.self) {
                                                Text($0, format: .number)
                                            }
                                        }
                                        Picker("Maksymalna temperatura", selection: $maxTemperature) {
                                            ForEach(Array(stride(from: minTemperature, to: 50.0, by: 0.1)), id: \.self) {
                                                Text($0, format: .number)
                                            }
                                        }
                                    }
                                    Button("Zapisz"){
                                        Task {
                                            let res = await httpClient.updatePlant(Plant(id: plant.id, familiarName: plant.familiarName, location: plant.location, maxHumidity: plant.maxHumidity, minHumidity: plant.minHumidity, maxTemperature: maxTemperature, minTemperature: minTemperature, sensorName: plant.sensorName))
                                            if res {
                                                DispatchQueue.main.async {
                                                    editingTemperature = false
                                                }
                                                mqttManager.publish(with: "\(minTemperature)", on: "\(plant.sensorName)/minTemperature", retain: true)
                                                mqttManager.publish(with: "\(maxTemperature)", on: "\(plant.sensorName)/maxTemperature", retain: true)
                                            }
                                        }
                                        
                                    }
                                }
                                
                            }
                        }
                        .padding(20)
                    }
                }
                .background(
                    RoundedCorner(radius: 30, corners: [.topLeft, .topRight])
                        .fill(.white)
                        .ignoresSafeArea()
                )
            }
        }
        .onReceive(timer) { _ in
            lastRecord.value = record
        }
    }
}

struct PlantDetailedView_Previews: PreviewProvider {
    static var previews: some View {
        PlantDetailedView(mqttManager: MQTTManager(), plant: Plant(id: 1, familiarName: "kwiat", location: "kuchnia", maxHumidity: 70, minHumidity: 20, maxTemperature: 22.1, minTemperature: 18.3, sensorName: "test"), httpClient: HttpClient())
    }
}
