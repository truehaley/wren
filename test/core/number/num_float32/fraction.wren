System.print(123.fraction)      // expect: 0
System.print((-123).fraction)   // expect: -0
System.print(0.fraction)        // expect: 0
System.print((-0).fraction)     // expect: -0
System.print(0.123.fraction)    // expect: 0.123
System.print(12.3.fraction)     // expect: 0.30000019
System.print((-0.123).fraction) // expect: -0.123
System.print((-12.3).fraction)  // expect: -0.30000019

// Using 32-bit representation, a longer mantissa will lead to
// approximation.
System.print((1.23456789012345).fraction)  // expect: 0.23456788
System.print((-1.23456789012345).fraction)  // expect: -0.23456788

System.print((0.000000000000000000000000000000000000000001).fraction)  // expect: 1.0005271e-42
System.print((-0.000000000000000000000000000000000000000001).fraction)  // expect: -1.0005271e-42

System.print((0.000000000000000000000000000000000001).fraction)  // expect: 1e-36
System.print((-0.000000000000000000000000000000000001).fraction)  // expect: -1e-36

System.print((1.000000000000000000000000000000000000000001).fraction)  // expect: 0
System.print((-1.000000000000000000000000000000000000000001).fraction)  // expect: -0
