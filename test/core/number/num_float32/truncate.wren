System.print(123.truncate)      // expect: 123
System.print((-123).truncate)   // expect: -123
System.print(0.truncate)        // expect: 0
System.print((-0).truncate)     // expect: -0
System.print(0.123.truncate)    // expect: 0
System.print(12.3.truncate)     // expect: 12
System.print((-0.123).truncate) // expect: -0
System.print((-12.3).truncate)  // expect: -12

// Using 32-bit representation, values "beyond" those  two will lead to
// approximation.
System.print((12345678901234.5).truncate)   // expect: 1.2345679e+13
System.print((-12345678901234.5).truncate)  // expect: -1.2345679e+13

System.print((16777216.5).truncate) // expect: 16777216
System.print((-16777216.5).truncate) // expect: -16777216

System.print((16777217.5).truncate) // expect: 16777218
System.print((-16777217.5).truncate) // expect: -16777218

System.print((16777215.5).truncate) // expect: 16777216
System.print((-16777215.5).truncate) // expect: -16777216

System.print((16777214.5).truncate) // expect: 16777214
System.print((-16777214.5).truncate) // expect: -16777214
