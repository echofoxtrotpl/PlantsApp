//
//  RegisterView.swift
//  PlantsApp
//
//  Created by Sebastian Zdybiowski on 23/01/2023.
//

import SwiftUI

struct RegisterView: View {
    @State private var email: String = ""
    @State private var password: String = ""
    @State private var confirmPassword: String = ""
    @State private var signupError = false
    @State private var signupErrorContent = ""
    @State private var firstName: String = ""
    @State private var lastName: String = ""
    @ObservedObject var httpClient: HttpClient
    @Environment(\.dismiss) var dismiss
    
    var body: some View {
        VStack {
            Form {
                Section("e-mail") {
                    TextField("e-mail", text: $email)
                        .textContentType(.emailAddress)
                        .disableAutocorrection(true)
                }
                Section("hasło") {
                    SecureField("hasło", text: $password)
                        .textContentType(.password)
                        .disableAutocorrection(true)
                }
                Section("powtórz hasło") {
                    SecureField("hasło", text: $confirmPassword)
                        .textContentType(.password)
                        .disableAutocorrection(true)
                }
                Section("imię") {
                    TextField("imię", text: $firstName)
                        .disableAutocorrection(true)
                }
                Section("nazwisko") {
                    TextField("nazwisko", text: $lastName)
                        .disableAutocorrection(true)
                }
            }
            
            Button {
                Task {
                    await signup()
                }
            } label: {
                Text("Zarejestruj się ")
                    .frame(maxWidth: 180)
            }
            .tint(.black)
            .buttonStyle(.borderedProminent)
            .padding(.bottom, 5)
            
            if signupError {
                Section {
                    Text(signupErrorContent)
                        .frame(maxWidth: .infinity, alignment: .center)
                        .foregroundColor(.red)
                }
            }
        }
        .navigationTitle("Rejestracja")
    }
    
    func signup() async {
        if email.isEmpty || password.isEmpty || firstName.isEmpty || lastName.isEmpty {
            signupErrorContent = "Wszystkie pola muszą być uzupełnione"
            signupError = true
            return
        }
        if password != confirmPassword {
            signupErrorContent = "Wprowadzone hasła są różne"
            signupError = true
            return
        }
        if password.count < 6 {
            signupErrorContent = "Hasło musi zawierać przynajmniej 6 znaków"
            signupError = true
            return
        }
        
        if !(await httpClient.signup(dto: SignupDto(email: email, password: password, firstName: firstName, lastName: lastName))) {
            signupErrorContent = "Ten adres e-mail jest już zajety"
            signupError = true
            return
        }
        dismiss()
    }
}

struct RegisterView_Previews: PreviewProvider {
    static var previews: some View {
        RegisterView(httpClient: HttpClient())
    }
}
