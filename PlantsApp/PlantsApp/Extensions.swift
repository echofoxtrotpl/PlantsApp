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

extension CodableWrapper: RawRepresentable {
    
    typealias RawValue = String
    
    var rawValue: RawValue {
        let encoder = JSONEncoder()
        encoder.dateEncodingStrategy = .iso8601
        guard
            let data = try? encoder.encode(value),
            let string = String(data: data, encoding: .utf8)
        else {
            return ""
        }
        return string
    }
    
    init?(rawValue: RawValue) {
        let decoder = JSONDecoder()
        decoder.dateDecodingStrategy = .iso8601
        guard
            let data = rawValue.data(using: .utf8),
            let decoded = try? decoder.decode(Value.self, from: data)
        else {
            return nil
        }
        value = decoded
    }
}
