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
    
    let timer = Timer.publish(every: 5, on: .main, in: .common).autoconnect()
    
    init(mqttManager: MQTTManager, plant: Plant) {
        self.plant = plant
        self._lastRecord = AppStorage(wrappedValue: .init(value: RecordStruct(temperature: 0.0, humidity: 0, updatedAt: Date(), sensorName: plant.sensorName)), plant.sensorName)
        self.mqttManager = mqttManager;
    }
    
    private var record: RecordStruct {
        get {
            mqttManager.records[plant.sensorName] ?? lastRecord.value
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
                        .padding(30)
                        VStack(alignment: .leading, spacing: 30){
                            HStack{
                                Text("Idealne warunki")
                                    .font(.title2)
                                    .bold()
                                    .padding(.bottom, 5)
                            }
                            HStack{
                                Text("Wilgotność")
                                Spacer()
                                Text("\(plant.minHumidity)% - \(plant.maxHumidity)%")
                            }
                            HStack{
                                Text("Temperatura")
                                Spacer()
                                Text("\(String(format: "%.1f°C", plant.minTemperature)) - \(String(format: "%.1f°C", plant.maxTemperature))")
                            }
                        }
                        .padding(30)
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
        PlantDetailedView(mqttManager: MQTTManager(), plant: Plant(id: 1, familiarName: "kwiat", location: "kuchnia", maxHumidity: 70, minHumidity: 20, maxTemperature: 22.1, minTemperature: 18.3, sensorName: "test"))
    }
}
