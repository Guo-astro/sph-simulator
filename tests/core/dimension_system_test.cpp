/**
 * @file dimension_system_test.cpp
 * @brief BDD-style tests for dimension-agnostic vector system
 * 
 * Following TDD/BDD approach:
 * - SCENARIO: High-level behavior description
 * - GIVEN: Initial state
 * - WHEN: Action taken
 * - THEN: Expected outcome
 */

#include "../bdd_helpers.hpp"
#include "../../include/core/vector.hpp"
#include <cmath>

using namespace sph;

FEATURE(DimensionAgnosticVectorSystem) {

SCENARIO(VectorConstruction, WorksForAllDimensions) {
    GIVEN("a default-constructed 1D vector") {
        Vector1D v;
        
        THEN("it should be initialized to zero") {
            EXPECT_EQ(v[0], 0.0;
        }
    }
    
    GIVEN("a 1D vector with single value") {
        Vector1D v(5.0);
        
        THEN("it should store the value correctly") {
            EXPECT_EQ(v[0], 5.0;
        }
    }
    
    GIVEN("a 2D vector with two values") {
        Vector2D v(3.0, 4.0);
        
        THEN("it should store both values correctly") {
            EXPECT_EQ(v[0], 3.0;
            EXPECT_EQ(v[1], 4.0;
        }
    }
    
    GIVEN("a 3D vector with three values") {
        Vector3D v(1.0, 2.0, 3.0);
        
        THEN("it should store all values correctly") {
            EXPECT_EQ(v[0], 1.0;
            EXPECT_EQ(v[1], 2.0;
            EXPECT_EQ(v[2], 3.0;
        }
    }
    
    GIVEN("a vector constructed from another vector") {
        Vector2D original(5.0, 6.0);
        Vector2D copy(original);
        
        THEN("it should have the same values") {
            EXPECT_EQ(copy[0], 5.0;
            EXPECT_EQ(copy[1], 6.0;
        }
        
        WHEN("the original is modified") {
            original[0] = 10.0;
            
            THEN("the copy should remain unchanged") {
                EXPECT_EQ(copy[0], 5.0;
            }
        }
    }
}

SCENARIO(VectorArithmetic, WorksCorrectly) {
    GIVEN("two 1D vectors") {
        Vector1D v1(5.0);
        Vector1D v2(3.0);
        
        WHEN("adding the vectors") {
            Vector1D result = v1 + v2;
            
            THEN("the result should be element-wise sum") {
                EXPECT_EQ(result[0], 8.0;
            }
        }
        
        WHEN("subtracting the vectors") {
            Vector1D result = v1 - v2;
            
            THEN("the result should be element-wise difference") {
                EXPECT_EQ(result[0], 2.0;
            }
        }
        
        WHEN("multiplying by a scalar") {
            Vector1D result = v1 * 2.0;
            
            THEN("all components should be scaled") {
                EXPECT_EQ(result[0], 10.0;
            }
        }
        
        WHEN("dividing by a scalar") {
            Vector1D result = v1 / 2.0;
            
            THEN("all components should be divided") {
                EXPECT_EQ(result[0], 2.5;
            }
        }
    }
    
    GIVEN("two 2D vectors") {
        Vector2D v1(1.0, 2.0);
        Vector2D v2(3.0, 4.0);
        
        WHEN("adding the vectors") {
            Vector2D result = v1 + v2;
            
            THEN("the result should be element-wise sum") {
                EXPECT_EQ(result[0], 4.0;
                EXPECT_EQ(result[1], 6.0;
            }
        }
        
        WHEN("negating a vector") {
            Vector2D result = -v1;
            
            THEN("all components should be negated") {
                EXPECT_EQ(result[0], -1.0;
                EXPECT_EQ(result[1], -2.0;
            }
        }
    }
    
    GIVEN("two 3D vectors") {
        Vector3D v1(1.0, 0.0, 0.0);
        Vector3D v2(0.0, 1.0, 0.0);
        
        WHEN("adding the vectors") {
            Vector3D result = v1 + v2;
            
            THEN("the result should combine components") {
                EXPECT_EQ(result[0], 1.0;
                EXPECT_EQ(result[1], 1.0;
                EXPECT_EQ(result[2], 0.0;
            }
        }
    }
}

SCENARIO(InnerProduct, WorksForAllDimensions) {
    GIVEN("two 1D vectors") {
        Vector1D v1(5.0);
        Vector1D v2(3.0);
        
        WHEN("computing inner product") {
            real dot = inner_product(v1, v2);
            
            THEN("result should be 15.0") {
                EXPECT_EQ(dot, 15.0;
            }
        }
    }
    
    GIVEN("two 2D vectors") {
        Vector2D v1(1.0, 2.0);
        Vector2D v2(3.0, 4.0);
        
        WHEN("computing inner product") {
            real dot = inner_product(v1, v2);
            
            THEN("result should be 1*3 + 2*4 = 11") {
                EXPECT_EQ(dot, 11.0;
            }
        }
    }
    
    GIVEN("two 3D vectors") {
        Vector3D v1(1.0, 2.0, 3.0);
        Vector3D v2(4.0, 5.0, 6.0);
        
        WHEN("computing inner product") {
            real dot = inner_product(v1, v2);
            
            THEN("result should be 1*4 + 2*5 + 3*6 = 32") {
                EXPECT_EQ(dot, 32.0;
            }
        }
    }
    
    GIVEN("orthogonal 2D vectors") {
        Vector2D v1(1.0, 0.0);
        Vector2D v2(0.0, 1.0);
        
        WHEN("computing inner product") {
            real dot = inner_product(v1, v2);
            
            THEN("result should be zero") {
                EXPECT_EQ(dot, 0.0;
            }
        }
    }
}

SCENARIO(VectorMagnitude, AndDistanceCalculations) {
    GIVEN("a 1D vector") {
        Vector1D v(3.0);
        
        WHEN("computing squared magnitude") {
            real mag2 = abs2(v);
            
            THEN("result should be 9.0") {
                EXPECT_EQ(mag2, 9.0;
            }
        }
        
        WHEN("computing magnitude") {
            real mag = abs(v);
            
            THEN("result should be 3.0") {
                EXPECT_EQ(mag, 3.0;
            }
        }
    }
    
    GIVEN("a 2D vector") {
        Vector2D v(3.0, 4.0);
        
        WHEN("computing squared magnitude") {
            real mag2 = abs2(v);
            
            THEN("result should be 9 + 16 = 25") {
                EXPECT_EQ(mag2, 25.0;
            }
        }
        
        WHEN("computing magnitude") {
            real mag = abs(v);
            
            THEN("result should be 5.0") {
                EXPECT_EQ(mag, 5.0;
            }
        }
    }
    
    GIVEN("a 3D vector") {
        Vector3D v(2.0, 3.0, 6.0);
        
        WHEN("computing magnitude") {
            real mag = abs(v);
            
            THEN("result should be 7.0") {
                EXPECT_EQ(mag, 7.0;
            }
        }
    }
    
    GIVEN("two 2D vectors") {
        Vector2D v1(1.0, 2.0);
        Vector2D v2(4.0, 6.0);
        
        WHEN("computing distance") {
            real dist = distance(v1, v2);
            
            THEN("result should be 5.0") {
                EXPECT_EQ(dist, 5.0;
            }
        }
    }
}

SCENARIO(CompoundAssignment, OperatorsWorkCorrectly) {
    GIVEN("a 2D vector") {
        Vector2D v(1.0, 2.0);
        
        WHEN("using += with another vector") {
            Vector2D other(3.0, 4.0);
            v += other;
            
            THEN("vector should be updated in place") {
                EXPECT_EQ(v[0], 4.0;
                EXPECT_EQ(v[1], 6.0;
            }
        }
        
        WHEN("using -= with another vector") {
            Vector2D v2(10.0, 20.0);
            Vector2D other(5.0, 8.0);
            v2 -= other;
            
            THEN("vector should be updated in place") {
                EXPECT_EQ(v2[0], 5.0;
                EXPECT_EQ(v2[1], 12.0;
            }
        }
        
        WHEN("using *= with a scalar") {
            Vector2D v3(2.0, 3.0);
            v3 *= 2.0;
            
            THEN("all components should be scaled") {
                EXPECT_EQ(v3[0], 4.0;
                EXPECT_EQ(v3[1], 6.0;
            }
        }
        
        WHEN("using /= with a scalar") {
            Vector2D v4(10.0, 20.0);
            v4 /= 2.0;
            
            THEN("all components should be divided") {
                EXPECT_EQ(v4[0], 5.0;
                EXPECT_EQ(v4[1], 10.0;
            }
        }
    }
}

SCENARIO(VectorAssignment, Operators) {
    GIVEN("a 2D vector") {
        Vector2D v(1.0, 2.0);
        
        WHEN("assigned from another vector") {
            Vector2D other(5.0, 6.0);
            v = other;
            
            THEN("it should have the new values") {
                EXPECT_EQ(v[0], 5.0;
                EXPECT_EQ(v[1], 6.0;
            }
        }
        
        WHEN("assigned from a scalar") {
            v = 3.0;
            
            THEN("all components should equal the scalar") {
                EXPECT_EQ(v[0], 3.0;
                EXPECT_EQ(v[1], 3.0;
            }
        }
    }
}

SCENARIO(DimensionSpecific, Operations) {
    GIVEN("2D vectors for cross product (z-component)") {
        Vector2D v1(1.0, 0.0);
        Vector2D v2(0.0, 1.0);
        
        WHEN("computing 2D vector product") {
            real cross_z = vector_product(v1, v2);
            
            THEN("result should be 1.0") {
                EXPECT_EQ(cross_z, 1.0;
            }
        }
    }
    
    GIVEN("parallel 2D vectors") {
        Vector2D v1(2.0, 4.0);
        Vector2D v2(1.0, 2.0);
        
        WHEN("computing 2D vector product") {
            real cross_z = vector_product(v1, v2);
            
            THEN("result should be 0") {
                EXPECT_EQ(cross_z, 0.0;
            }
        }
    }
}

SCENARIO(CompileTimeDimension, Information) {
    GIVEN("different vector types") {
        THEN("they should know their dimensions at compile time") {
            EXPECT_EQ(Vector1D::dimension, 1;
            EXPECT_EQ(Vector2D::dimension, 2;
            EXPECT_EQ(Vector3D::dimension, 3;
        }
    }
}

} // FEATURE(DimensionAgnosticVectorSystem)
