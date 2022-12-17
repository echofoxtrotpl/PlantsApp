//
//  ProgressChart.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import SwiftUI

struct ProgressChart: View {
    var progress: Double = 0.71
    var maxValue: Double = 0.7
    var minValue: Double = 0.3
    
    var body: some View {
        ZStack {
            Circle()
                .stroke(lineWidth: 15.0)
                .opacity(0.3)
                .foregroundColor((progress > maxValue || progress < minValue) ? .red : .green)
            
            Circle()
                .trim(from: 0.0, to: CGFloat(min(progress, 1.0)))
                .stroke(style: StrokeStyle(lineWidth: 10.0, lineCap: .round, lineJoin: .round))
                .foregroundColor((progress > maxValue || progress < minValue) ? .red : .green)
                .rotationEffect(Angle(degrees: 270.0))
                .animation(.easeInOut, value: progress)
            Text(String(format: "%.0f %%", min(progress, 1.0)*100.0))
                .font(.title2)
                .bold()
        }
    }
}

struct ProgressChart_Previews: PreviewProvider {
    static var previews: some View {
        ProgressChart()
    }
}
