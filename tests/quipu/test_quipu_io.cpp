#include <fstream>
#include <string>
#include <cstdint>

#include "gtest/gtest.h"
#include "glm/glm.hpp"

#include "vira/math.hpp"
#include "vira/quipu/class_ids.hpp"
#include "../../source/vira/quipu/quipu_io.hpp"

template <int N, typename Tp1 ,typename Tp2>
void vecIOTest() {
    // Initialize:
    std::string fileName = "test.qpu";
    uint32_t writtenVecSize = static_cast<uint32_t>(sizeof(vira::ViraClassID) + sizeof(uint8_t) + N*sizeof(Tp1));
    glm::vec<N, Tp1, glm::highp> outVec;
    glm::vec<N, Tp2, glm::highp> inVec;
    for (int i = 0; i < N; i++) {
        outVec[i] = static_cast<Tp1>(2.3*i + 4.5);
    }

    // Write the vector:
    std::ofstream outFile(fileName, std::ios::binary);
    uint32_t bufferSize = vira::QuipuIO::writeVec(outFile, outVec);
    outFile.close();
    ASSERT_TRUE(bufferSize == writtenVecSize);
    
    // Read the vector:
    std::ifstream inFile(fileName, std::ios::binary);
    vira::QuipuIO::readVec(inFile, inVec);
    inFile.close();

    // Assert they are equal:
    bool areEqual = true;
    for (int i = 0; i < N; i++) {
        areEqual = areEqual && (static_cast<float>(inVec[i]) == static_cast<float>(outVec[i]));
    }
    ASSERT_TRUE(areEqual);
};

template <int N, int M, typename Tp1, typename Tp2>
void matIOTest() {
    // Initialize:
    std::string fileName = "test.qpu";
    uint32_t writtenMatSize = static_cast<uint32_t>(sizeof(vira::ViraClassID) + 2*sizeof(uint8_t) + (N * M * sizeof(Tp1)));
    glm::mat<N, M, Tp1, glm::highp> outMat;
    glm::mat<N, M, Tp2, glm::highp> inMat;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            outMat[i][j] = static_cast<Tp1>(2.3 * i + 4.5 *j + 6.7);
        }
    }

    // Write the matrix:
    std::ofstream outFile(fileName, std::ios::binary);
    uint32_t bufferSize = vira::QuipuIO::writeMat(outFile, outMat);
    outFile.close();
    ASSERT_TRUE(bufferSize == writtenMatSize);

    // Read the matrix:
    std::ifstream inFile(fileName, std::ios::binary);
    vira::QuipuIO::readMat(inFile, inMat);
    inFile.close();

    // Assert they are equal:
    bool areEqual = true;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < M; j++) {
            areEqual = areEqual && (static_cast<float>(inMat[i][j]) == static_cast<float>(outMat[i][j]));
        }
    }
    ASSERT_TRUE(areEqual);
}

TEST(TestQuipuIO, vec1_f_to_f_IO) { vecIOTest<1, float, float>(); };
TEST(TestQuipuIO, vec1_f_to_d_IO) { vecIOTest<1, float, double>(); };
TEST(TestQuipuIO, vec1_d_to_d_IO) { vecIOTest<1, double, double>(); };
TEST(TestQuipuIO, vec1_d_to_f_IO) { vecIOTest<1, double, float>(); };

TEST(TestQuipuIO, vec2_f_to_f_IO) { vecIOTest<2, float, float>(); };
TEST(TestQuipuIO, vec2_f_to_d_IO) { vecIOTest<2, float, double>(); };
TEST(TestQuipuIO, vec2_d_to_d_IO) { vecIOTest<2, double, double>(); };
TEST(TestQuipuIO, vec2_d_to_f_IO) { vecIOTest<2, double, float>(); };

TEST(TestQuipuIO, vec3_f_to_f_IO) { vecIOTest<3, float, float>(); };
TEST(TestQuipuIO, vec3_f_to_d_IO) { vecIOTest<3, float, double>(); };
TEST(TestQuipuIO, vec3_d_to_d_IO) { vecIOTest<3, double, double>(); };
TEST(TestQuipuIO, vec3_d_to_f_IO) { vecIOTest<3, double, float>(); };

TEST(TestQuipuIO, vec4_f_to_f_IO) { vecIOTest<4, float, float>(); };
TEST(TestQuipuIO, vec4_f_to_d_IO) { vecIOTest<4, float, double>(); };
TEST(TestQuipuIO, vec4_d_to_d_IO) { vecIOTest<4, double, double>(); };
TEST(TestQuipuIO, vec4_d_to_f_IO) { vecIOTest<4, double, float>(); };

TEST(TestQuipuIO, mat2_f_to_f_IO) { matIOTest<2, 2, float, float>(); };
TEST(TestQuipuIO, mat2_f_to_d_IO) { matIOTest<2, 2, float, double>(); };
TEST(TestQuipuIO, mat2_d_to_d_IO) { matIOTest<2, 2, double, double>(); };
TEST(TestQuipuIO, mat2_d_to_f_IO) { matIOTest<2, 2, double, float>(); };

TEST(TestQuipuIO, mat3_f_to_f_IO) { matIOTest<3, 3, float, float>(); };
TEST(TestQuipuIO, mat3_f_to_d_IO) { matIOTest<3, 3, float, double>(); };
TEST(TestQuipuIO, mat3_d_to_d_IO) { matIOTest<3, 3, double, double>(); };
TEST(TestQuipuIO, mat3_d_to_f_IO) { matIOTest<3, 3, double, float>(); };

TEST(TestQuipuIO, mat4_f_to_f_IO) { matIOTest<4, 4, float, float>(); };
TEST(TestQuipuIO, mat4_f_to_d_IO) { matIOTest<4, 4, float, double>(); };
TEST(TestQuipuIO, mat4_d_to_d_IO) { matIOTest<4, 4, double, double>(); };
TEST(TestQuipuIO, mat4_d_to_f_IO) { matIOTest<4, 4, double, float>(); };