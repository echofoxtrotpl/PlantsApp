//
//  Extensions.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/10/2022.
//

import Foundation
import SwiftUI

extension Color {
    public init(red: Int, green: Int, blue: Int) {
        self = Color(red: Double(red)/255.0,
                     green: Double(green)/255.0,
                     blue: Double(blue)/255.0)
    }
}
