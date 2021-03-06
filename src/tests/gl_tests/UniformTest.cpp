//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

#include <array>
#include <cmath>

using namespace angle;

namespace
{

class UniformTest : public ANGLETest
{
  protected:
    UniformTest() : mProgram(0), mUniformFLocation(-1), mUniformILocation(-1), mUniformBLocation(-1)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        const std::string &vertexShader = "void main() { gl_Position = vec4(1); }";
        const std::string &fragShader =
            "precision mediump float;\n"
            "uniform float uniF;\n"
            "uniform int uniI;\n"
            "uniform bool uniB;\n"
            "uniform bool uniBArr[4];\n"
            "void main() {\n"
            "  gl_FragColor = vec4(uniF + float(uniI));\n"
            "  gl_FragColor += vec4(uniB ? 1.0 : 0.0);\n"
            "  gl_FragColor += vec4(uniBArr[0] ? 1.0 : 0.0);\n"
            "}";

        mProgram = CompileProgram(vertexShader, fragShader);
        ASSERT_NE(mProgram, 0u);

        mUniformFLocation = glGetUniformLocation(mProgram, "uniF");
        ASSERT_NE(mUniformFLocation, -1);

        mUniformILocation = glGetUniformLocation(mProgram, "uniI");
        ASSERT_NE(mUniformILocation, -1);

        mUniformBLocation = glGetUniformLocation(mProgram, "uniB");
        ASSERT_NE(mUniformBLocation, -1);

        ASSERT_GL_NO_ERROR();
    }

    void TearDown() override
    {
        glDeleteProgram(mProgram);
        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLint mUniformFLocation;
    GLint mUniformILocation;
    GLint mUniformBLocation;
};

TEST_P(UniformTest, GetUniformNoCurrentProgram)
{
    glUseProgram(mProgram);
    glUniform1f(mUniformFLocation, 1.0f);
    glUniform1i(mUniformILocation, 1);
    glUseProgram(0);

    GLfloat f;
    glGetnUniformfvEXT(mProgram, mUniformFLocation, 4, &f);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1.0f, f);

    glGetUniformfv(mProgram, mUniformFLocation, &f);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1.0f, f);

    GLint i;
    glGetnUniformivEXT(mProgram, mUniformILocation, 4, &i);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1, i);

    glGetUniformiv(mProgram, mUniformILocation, &i);
    ASSERT_GL_NO_ERROR();
    EXPECT_EQ(1, i);
}

TEST_P(UniformTest, UniformArrayLocations)
{
    // TODO(geofflang): Figure out why this is broken on Intel OpenGL
    if (IsIntel() && getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE)
    {
        std::cout << "Test skipped on Intel OpenGL." << std::endl;
        return;
    }

    const std::string vertexShader = SHADER_SOURCE
    (
        precision mediump float;
        uniform float uPosition[4];
        void main(void)
        {
            gl_Position = vec4(uPosition[0], uPosition[1], uPosition[2], uPosition[3]);
        }
    );

    const std::string fragShader = SHADER_SOURCE
    (
        precision mediump float;
        uniform float uColor[4];
        void main(void)
        {
            gl_FragColor = vec4(uColor[0], uColor[1], uColor[2], uColor[3]);
        }
    );

    GLuint program = CompileProgram(vertexShader, fragShader);
    ASSERT_NE(program, 0u);

    // Array index zero should be equivalent to the un-indexed uniform
    EXPECT_NE(-1, glGetUniformLocation(program, "uPosition"));
    EXPECT_EQ(glGetUniformLocation(program, "uPosition"), glGetUniformLocation(program, "uPosition[0]"));

    EXPECT_NE(-1, glGetUniformLocation(program, "uColor"));
    EXPECT_EQ(glGetUniformLocation(program, "uColor"), glGetUniformLocation(program, "uColor[0]"));

    // All array uniform locations should be unique
    GLint positionLocations[4] =
    {
        glGetUniformLocation(program, "uPosition[0]"),
        glGetUniformLocation(program, "uPosition[1]"),
        glGetUniformLocation(program, "uPosition[2]"),
        glGetUniformLocation(program, "uPosition[3]"),
    };

    GLint colorLocations[4] =
    {
        glGetUniformLocation(program, "uColor[0]"),
        glGetUniformLocation(program, "uColor[1]"),
        glGetUniformLocation(program, "uColor[2]"),
        glGetUniformLocation(program, "uColor[3]"),
    };

    for (size_t i = 0; i < 4; i++)
    {
        EXPECT_NE(-1, positionLocations[i]);
        EXPECT_NE(-1, colorLocations[i]);

        for (size_t j = i + 1; j < 4; j++)
        {
            EXPECT_NE(positionLocations[i], positionLocations[j]);
            EXPECT_NE(colorLocations[i], colorLocations[j]);
        }
    }

    glDeleteProgram(program);
}

// Test that float to integer GetUniform rounds values correctly.
TEST_P(UniformTest, FloatUniformStateQuery)
{
    std::vector<double> inValues;
    std::vector<GLfloat> expectedFValues;
    std::vector<GLint> expectedIValues;

    double intMaxD = static_cast<double>(std::numeric_limits<GLint>::max());
    double intMinD = static_cast<double>(std::numeric_limits<GLint>::min());

    // TODO(jmadill): Investigate rounding of .5
    inValues.push_back(-1.0);
    inValues.push_back(-0.6);
    // inValues.push_back(-0.5); // undefined behaviour?
    inValues.push_back(-0.4);
    inValues.push_back(0.0);
    inValues.push_back(0.4);
    // inValues.push_back(0.5); // undefined behaviour?
    inValues.push_back(0.6);
    inValues.push_back(1.0);
    inValues.push_back(999999.2);
    inValues.push_back(intMaxD * 2.0);
    inValues.push_back(intMaxD + 1.0);
    inValues.push_back(intMinD * 2.0);
    inValues.push_back(intMinD - 1.0);

    for (double value : inValues)
    {
        expectedFValues.push_back(static_cast<GLfloat>(value));

        double clampedValue = std::max(intMinD, std::min(intMaxD, value));
        double rounded = round(clampedValue);
        expectedIValues.push_back(static_cast<GLint>(rounded));
    }

    glUseProgram(mProgram);
    ASSERT_GL_NO_ERROR();

    for (size_t index = 0; index < inValues.size(); ++index)
    {
        GLfloat inValue       = static_cast<GLfloat>(inValues[index]);
        GLfloat expectedValue = expectedFValues[index];

        glUniform1f(mUniformFLocation, inValue);
        GLfloat testValue;
        glGetUniformfv(mProgram, mUniformFLocation, &testValue);
        ASSERT_GL_NO_ERROR();
        EXPECT_EQ(expectedValue, testValue);
    }

    for (size_t index = 0; index < inValues.size(); ++index)
    {
        GLfloat inValue     = static_cast<GLfloat>(inValues[index]);
        GLint expectedValue = expectedIValues[index];

        glUniform1f(mUniformFLocation, inValue);
        GLint testValue;
        glGetUniformiv(mProgram, mUniformFLocation, &testValue);
        ASSERT_GL_NO_ERROR();
        EXPECT_EQ(expectedValue, testValue);
    }
}

// Test that integer to float GetUniform rounds values correctly.
TEST_P(UniformTest, IntUniformStateQuery)
{
    std::vector<GLint> inValues;
    std::vector<GLint> expectedIValues;
    std::vector<GLfloat> expectedFValues;

    GLint intMax = std::numeric_limits<GLint>::max();
    GLint intMin = std::numeric_limits<GLint>::min();

    inValues.push_back(-1);
    inValues.push_back(0);
    inValues.push_back(1);
    inValues.push_back(999999);
    inValues.push_back(intMax);
    inValues.push_back(intMax - 1);
    inValues.push_back(intMin);
    inValues.push_back(intMin + 1);

    for (GLint value : inValues)
    {
        expectedIValues.push_back(value);
        expectedFValues.push_back(static_cast<GLfloat>(value));
    }

    glUseProgram(mProgram);
    ASSERT_GL_NO_ERROR();

    for (size_t index = 0; index < inValues.size(); ++index)
    {
        GLint inValue       = inValues[index];
        GLint expectedValue = expectedIValues[index];

        glUniform1i(mUniformILocation, inValue);
        GLint testValue;
        glGetUniformiv(mProgram, mUniformILocation, &testValue);
        ASSERT_GL_NO_ERROR();
        EXPECT_EQ(expectedValue, testValue);
    }

    for (size_t index = 0; index < inValues.size(); ++index)
    {
        GLint inValue         = inValues[index];
        GLfloat expectedValue = expectedFValues[index];

        glUniform1i(mUniformILocation, inValue);
        GLfloat testValue;
        glGetUniformfv(mProgram, mUniformILocation, &testValue);
        ASSERT_GL_NO_ERROR();
        EXPECT_EQ(expectedValue, testValue);
    }
}

// Test that queries of boolean uniforms round correctly.
TEST_P(UniformTest, BooleanUniformStateQuery)
{
    glUseProgram(mProgram);
    GLint intValue     = 0;
    GLfloat floatValue = 0.0f;

    // Calling Uniform1i
    glUniform1i(mUniformBLocation, GL_FALSE);

    glGetUniformiv(mProgram, mUniformBLocation, &intValue);
    EXPECT_EQ(0, intValue);

    glGetUniformfv(mProgram, mUniformBLocation, &floatValue);
    EXPECT_EQ(0.0f, floatValue);

    glUniform1i(mUniformBLocation, GL_TRUE);

    glGetUniformiv(mProgram, mUniformBLocation, &intValue);
    EXPECT_EQ(1, intValue);

    glGetUniformfv(mProgram, mUniformBLocation, &floatValue);
    EXPECT_EQ(1.0f, floatValue);

    // Calling Uniform1f
    glUniform1f(mUniformBLocation, 0.0f);

    glGetUniformiv(mProgram, mUniformBLocation, &intValue);
    EXPECT_EQ(0, intValue);

    glGetUniformfv(mProgram, mUniformBLocation, &floatValue);
    EXPECT_EQ(0.0f, floatValue);

    glUniform1f(mUniformBLocation, 1.0f);

    glGetUniformiv(mProgram, mUniformBLocation, &intValue);
    EXPECT_EQ(1, intValue);

    glGetUniformfv(mProgram, mUniformBLocation, &floatValue);
    EXPECT_EQ(1.0f, floatValue);

    ASSERT_GL_NO_ERROR();
}

// Test queries for arrays of boolean uniforms.
TEST_P(UniformTest, BooleanArrayUniformStateQuery)
{
    glUseProgram(mProgram);
    GLint boolValuesi[4]   = {0, 1, 0, 1};
    GLfloat boolValuesf[4] = {0, 1, 0, 1};

    GLint locations[4] = {
        glGetUniformLocation(mProgram, "uniBArr"),
        glGetUniformLocation(mProgram, "uniBArr[1]"),
        glGetUniformLocation(mProgram, "uniBArr[2]"),
        glGetUniformLocation(mProgram, "uniBArr[3]"),
    };

    // Calling Uniform1iv
    glUniform1iv(locations[0], 4, boolValuesi);

    for (unsigned int idx = 0; idx < 4; ++idx)
    {
        int value = -1;
        glGetUniformiv(mProgram, locations[idx], &value);
        EXPECT_EQ(boolValuesi[idx], value);
    }

    for (unsigned int idx = 0; idx < 4; ++idx)
    {
        float value = -1.0f;
        glGetUniformfv(mProgram, locations[idx], &value);
        EXPECT_EQ(boolValuesf[idx], value);
    }

    // Calling Uniform1fv
    glUniform1fv(locations[0], 4, boolValuesf);

    for (unsigned int idx = 0; idx < 4; ++idx)
    {
        int value = -1;
        glGetUniformiv(mProgram, locations[idx], &value);
        EXPECT_EQ(boolValuesi[idx], value);
    }

    for (unsigned int idx = 0; idx < 4; ++idx)
    {
        float value = -1.0f;
        glGetUniformfv(mProgram, locations[idx], &value);
        EXPECT_EQ(boolValuesf[idx], value);
    }

    ASSERT_GL_NO_ERROR();
}

class UniformTestES3 : public ANGLETest
{
  protected:
    UniformTestES3() : mProgram(0) {}

    void SetUp() override
    {
        ANGLETest::SetUp();
    }

    void TearDown() override
    {
        if (mProgram != 0)
        {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }
    }

    GLuint mProgram;
};

// Test queries for transposed arrays of non-square matrix uniforms.
TEST_P(UniformTestES3, TranposedMatrixArrayUniformStateQuery)
{
    const std::string &vertexShader =
        "#version 300 es\n"
        "void main() { gl_Position = vec4(1); }";
    const std::string &fragShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform mat3x2 uniMat3x2[5];\n"
        "out vec4 color;\n"
        "void main() {\n"
        "  color = vec4(uniMat3x2[0][0][0]);\n"
        "}";

    mProgram = CompileProgram(vertexShader, fragShader);
    ASSERT_NE(mProgram, 0u);

    glUseProgram(mProgram);

    std::vector<GLfloat> transposedValues;

    for (size_t arrayElement = 0; arrayElement < 5; ++arrayElement)
    {
        transposedValues.push_back(1.0f + arrayElement);
        transposedValues.push_back(3.0f + arrayElement);
        transposedValues.push_back(5.0f + arrayElement);
        transposedValues.push_back(2.0f + arrayElement);
        transposedValues.push_back(4.0f + arrayElement);
        transposedValues.push_back(6.0f + arrayElement);
    }

    // Setting as a clump
    GLint baseLocation = glGetUniformLocation(mProgram, "uniMat3x2");
    ASSERT_NE(-1, baseLocation);

    glUniformMatrix3x2fv(baseLocation, 5, GL_TRUE, &transposedValues[0]);

    for (size_t arrayElement = 0; arrayElement < 5; ++arrayElement)
    {
        std::stringstream nameStr;
        nameStr << "uniMat3x2[" << arrayElement << "]";
        std::string name = nameStr.str();
        GLint location = glGetUniformLocation(mProgram, name.c_str());
        ASSERT_NE(-1, location);

        std::vector<GLfloat> sequentialValues(6, 0);
        glGetUniformfv(mProgram, location, &sequentialValues[0]);

        ASSERT_GL_NO_ERROR();

        for (size_t comp = 0; comp < 6; ++comp)
        {
            EXPECT_EQ(static_cast<GLfloat>(comp + 1 + arrayElement), sequentialValues[comp]);
        }
    }
}

// Check that trying setting too many elements of an array doesn't overflow
TEST_P(UniformTestES3, OverflowArray)
{
    const std::string &vertexShader =
        "#version 300 es\n"
        "void main() { gl_Position = vec4(1); }";
    const std::string &fragShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform float uniF[5];\n"
        "uniform mat3x2 uniMat3x2[5];\n"
        "out vec4 color;\n"
        "void main() {\n"
        "  color = vec4(uniMat3x2[0][0][0] + uniF[0]);\n"
        "}";

    mProgram = CompileProgram(vertexShader, fragShader);
    ASSERT_NE(mProgram, 0u);

    glUseProgram(mProgram);

    const size_t kOverflowSize = 10000;
    std::vector<GLfloat> values(10000 * 6);

    // Setting as a clump
    GLint floatLocation = glGetUniformLocation(mProgram, "uniF");
    ASSERT_NE(-1, floatLocation);
    GLint matLocation = glGetUniformLocation(mProgram, "uniMat3x2");
    ASSERT_NE(-1, matLocation);

    // Set too many float uniforms
    glUniform1fv(floatLocation, kOverflowSize, &values[0]);

    // Set too many matrix uniforms, transposed or not
    glUniformMatrix3x2fv(matLocation, kOverflowSize, GL_FALSE, &values[0]);
    glUniformMatrix3x2fv(matLocation, kOverflowSize, GL_TRUE, &values[0]);

    // Same checks but with offsets
    GLint floatLocationOffset = glGetUniformLocation(mProgram, "uniF[3]");
    ASSERT_NE(-1, floatLocationOffset);
    GLint matLocationOffset = glGetUniformLocation(mProgram, "uniMat3x2[3]");
    ASSERT_NE(-1, matLocationOffset);

    glUniform1fv(floatLocationOffset, kOverflowSize, &values[0]);
    glUniformMatrix3x2fv(matLocationOffset, kOverflowSize, GL_FALSE, &values[0]);
    glUniformMatrix3x2fv(matLocationOffset, kOverflowSize, GL_TRUE, &values[0]);
}

// Check setting a sampler uniform
TEST_P(UniformTest, Sampler)
{
    const std::string &vertShader =
        "uniform sampler2D tex2D;\n"
        "void main() {\n"
        "  gl_Position = vec4(0, 0, 0, 1);\n"
        "}";

    const std::string &fragShader =
        "precision mediump float;\n"
        "uniform sampler2D tex2D;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(tex2D, vec2(0, 0));\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertShader, fragShader);

    GLint location = glGetUniformLocation(program.get(), "tex2D");
    ASSERT_NE(-1, location);

    const GLint sampler[] = {0, 0, 0, 0};

    // before UseProgram
    glUniform1i(location, sampler[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    glUseProgram(program.get());

    // Uniform1i
    glUniform1i(location, sampler[0]);
    glUniform1iv(location, 1, sampler);
    EXPECT_GL_NO_ERROR();

    // Uniform{234}i
    glUniform2i(location, sampler[0], sampler[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform3i(location, sampler[0], sampler[0], sampler[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform4i(location, sampler[0], sampler[0], sampler[0], sampler[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform2iv(location, 1, sampler);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform3iv(location, 1, sampler);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform4iv(location, 1, sampler);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // Uniform{1234}f
    const GLfloat f[] = {0, 0, 0, 0};
    glUniform1f(location, f[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform2f(location, f[0], f[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform3f(location, f[0], f[0], f[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform4f(location, f[0], f[0], f[0], f[0]);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform1fv(location, 1, f);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform2fv(location, 1, f);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform3fv(location, 1, f);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    glUniform4fv(location, 1, f);
    EXPECT_GL_ERROR(GL_INVALID_OPERATION);

    // < 0 or >= max
    GLint tooHigh;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &tooHigh);
    constexpr GLint tooLow[] = {-1};
    glUniform1i(location, tooLow[0]);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
    glUniform1iv(location, 1, tooLow);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
    glUniform1i(location, tooHigh);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
    glUniform1iv(location, 1, &tooHigh);
    EXPECT_GL_ERROR(GL_INVALID_VALUE);
}

// Check that sampler uniforms only show up one time in the list
TEST_P(UniformTest, SamplerUniformsAppearOnce)
{
    int maxVertexTextureImageUnits = 0;
    glGetIntegerv(GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS, &maxVertexTextureImageUnits);

    if (maxVertexTextureImageUnits == 0)
    {
        std::cout << "Renderer doesn't support vertex texture fetch, skipping test" << std::endl;
        return;
    }

    const std::string &vertShader =
        "attribute vec2 position;\n"
        "uniform sampler2D tex2D;\n"
        "varying vec4 color;\n"
        "void main() {\n"
        "  gl_Position = vec4(position, 0, 1);\n"
        "  color = texture2D(tex2D, vec2(0));\n"
        "}";

    const std::string &fragShader =
        "precision mediump float;\n"
        "varying vec4 color;\n"
        "uniform sampler2D tex2D;\n"
        "void main() {\n"
        "  gl_FragColor = texture2D(tex2D, vec2(0)) + color;\n"
        "}";

    GLuint program = CompileProgram(vertShader, fragShader);
    ASSERT_NE(0u, program);

    GLint activeUniformsCount = 0;
    glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &activeUniformsCount);
    ASSERT_EQ(1, activeUniformsCount);

    GLint size       = 0;
    GLenum type      = GL_NONE;
    GLchar name[120] = {0};
    glGetActiveUniform(program, 0, 100, nullptr, &size, &type, name);
    EXPECT_EQ(1, size);
    EXPECT_GLENUM_EQ(GL_SAMPLER_2D, type);
    EXPECT_STREQ("tex2D", name);

    EXPECT_GL_NO_ERROR();

    glDeleteProgram(program);
}

template <typename T, typename GetUniformV>
void CheckOneElement(GetUniformV getUniformv,
                     GLuint program,
                     const std::string &name,
                     int components,
                     T canary)
{
    // The buffer getting the results has three chunks
    //  - A chunk to see underflows
    //  - A chunk that will hold the result
    //  - A chunk to see overflows for when components = kChunkSize
    static const size_t kChunkSize = 4;
    std::array<T, 3 * kChunkSize> buffer;
    buffer.fill(canary);

    GLint location = glGetUniformLocation(program, name.c_str());
    ASSERT_NE(location, -1);

    getUniformv(program, location, &buffer[kChunkSize]);
    for (size_t i = 0; i < kChunkSize; i++)
    {
        ASSERT_EQ(canary, buffer[i]);
    }
    for (size_t i = kChunkSize + components; i < buffer.size(); i++)
    {
        ASSERT_EQ(canary, buffer[i]);
    }
}

// Check that getting an element array doesn't return the whole array.
TEST_P(UniformTestES3, ReturnsOnlyOneArrayElement)
{
    static const size_t kArraySize = 4;
    struct UniformArrayInfo
    {
        UniformArrayInfo(std::string type, std::string name, int components)
            : type(type), name(name), components(components)
        {
        }
        std::string type;
        std::string name;
        int components;
    };

    // Check for various number of components and types
    std::vector<UniformArrayInfo> uniformArrays;
    uniformArrays.emplace_back("bool", "uBool", 1);
    uniformArrays.emplace_back("vec2", "uFloat", 2);
    uniformArrays.emplace_back("ivec3", "uInt", 3);
    uniformArrays.emplace_back("uvec4", "uUint", 4);

    std::ostringstream uniformStream;
    std::ostringstream additionStream;
    for (const auto &array : uniformArrays)
    {
        uniformStream << "uniform " << array.type << " " << array.name << "["
                      << ToString(kArraySize) << "];\n";

        // We need to make use of the uniforms or they get compiled out.
        for (int i = 0; i < 4; i++)
        {
            if (array.components == 1)
            {
                additionStream << " + float(" << array.name << "[" << i << "])";
            }
            else
            {
                for (int component = 0; component < array.components; component++)
                {
                    additionStream << " + float(" << array.name << "[" << i << "][" << component
                                   << "])";
                }
            }
        }
    }

    const std::string &vertexShader =
        "#version 300 es\n" +
        uniformStream.str() +
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(1.0" + additionStream.str() + ");\n"
        "}";

    const std::string &fragmentShader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "out vec4 color;\n"
        "void main ()\n"
        "{\n"
        "    color = vec4(1, 0, 0, 1);\n"
        "}";

    mProgram = CompileProgram(vertexShader, fragmentShader);
    ASSERT_NE(0u, mProgram);

    glUseProgram(mProgram);

    for (const auto &uniformArray : uniformArrays)
    {
        for (size_t index = 0; index < kArraySize; index++)
        {
            std::string strIndex = "[" + ToString(index) + "]";
            // Check all the different glGetUniformv functions
            CheckOneElement<float>(glGetUniformfv, mProgram, uniformArray.name + strIndex,
                                   uniformArray.components, 42.4242f);
            CheckOneElement<int>(glGetUniformiv, mProgram, uniformArray.name + strIndex,
                                 uniformArray.components, 0x7BADBED5);
            CheckOneElement<unsigned int>(glGetUniformuiv, mProgram, uniformArray.name + strIndex,
                                          uniformArray.components, 0xDEADBEEF);
        }
    }
}

class UniformTestES31 : public ANGLETest
{
  protected:
    UniformTestES31() : mProgram(0) {}

    void SetUp() override { ANGLETest::SetUp(); }

    void TearDown() override
    {
        if (mProgram != 0)
        {
            glDeleteProgram(mProgram);
            mProgram = 0;
        }
    }

    GLuint mProgram;
};

// Test that uniform locations get set correctly for structure members.
// ESSL 3.10.4 section 4.4.3.
TEST_P(UniformTestES31, StructLocationLayoutQualifier)
{
    const std::string &vertShader =
        "#version 310 es\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(0);\n"
        "}";

    const std::string &fragShader =
        "#version 310 es\n"
        "out highp vec4 my_FragColor;\n"
        "struct S\n"
        "{\n"
        "    highp float f;\n"
        "    highp float f2;\n"
        "};\n"
        "uniform layout(location=12) S uS;\n"
        "void main()\n"
        "{\n"
        "    my_FragColor = vec4(uS.f, uS.f2, 0, 1);\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertShader, fragShader);

    EXPECT_EQ(12, glGetUniformLocation(program.get(), "uS.f"));
    EXPECT_EQ(13, glGetUniformLocation(program.get(), "uS.f2"));
}

// Set uniform location with a layout qualifier in the fragment shader. The same uniform exists in
// the vertex shader, but doesn't have a location specified there.
TEST_P(UniformTestES31, UniformLocationInFragmentShader)
{
    const std::string &vertShader =
        "#version 310 es\n"
        "uniform highp sampler2D tex2D;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = texture(tex2D, vec2(0));\n"
        "}";

    const std::string &fragShader =
        "#version 310 es\n"
        "precision mediump float;\n"
        "out vec4 my_FragColor;\n"
        "uniform layout(location=12) highp sampler2D tex2D;\n"
        "void main()\n"
        "{\n"
        "    my_FragColor = texture(tex2D, vec2(0));\n"
        "}";

    ANGLE_GL_PROGRAM(program, vertShader, fragShader);

    EXPECT_EQ(12, glGetUniformLocation(program.get(), "tex2D"));
}

// Test two unused uniforms that have the same location.
// ESSL 3.10.4 section 4.4.3: "No two default-block uniform variables in the program can have the
// same location, even if they are unused, otherwise a compiler or linker error will be generated."
TEST_P(UniformTestES31, UnusedUniformsConflictingLocation)
{
    const std::string &vertShader =
        "#version 310 es\n"
        "uniform layout(location=12) highp sampler2D texA;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(0);\n"
        "}";

    const std::string &fragShader =
        "#version 310 es\n"
        "out highp vec4 my_FragColor;\n"
        "uniform layout(location=12) highp sampler2D texB;\n"
        "void main()\n"
        "{\n"
        "    my_FragColor = vec4(0);\n"
        "}";

    mProgram = CompileProgram(vertShader, fragShader);
    EXPECT_EQ(0u, mProgram);
}

// Test two unused uniforms that have overlapping locations once all array elements are taken into
// account.
// ESSL 3.10.4 section 4.4.3: "No two default-block uniform variables in the program can have the
// same location, even if they are unused, otherwise a compiler or linker error will be generated."
TEST_P(UniformTestES31, UnusedUniformArraysConflictingLocation)
{
    const std::string &vertShader =
        "#version 310 es\n"
        "uniform layout(location=11) highp vec4 uA[2];\n"
        "void main()\n"
        "{\n"
        "    gl_Position = vec4(0);\n"
        "}";

    const std::string &fragShader =
        "#version 310 es\n"
        "out highp vec4 my_FragColor;\n"
        "uniform layout(location=12) highp vec4 uB;\n"
        "void main()\n"
        "{\n"
        "    my_FragColor = vec4(0);\n"
        "}";

    mProgram = CompileProgram(vertShader, fragShader);
    EXPECT_EQ(0u, mProgram);
}

// Test a uniform struct containing a non-square matrix and a boolean.
// Minimal test case for a bug revealed by dEQP tests.
TEST_P(UniformTestES3, StructWithNonSquareMatrixAndBool)
{
    const std::string &vertShader =
        "#version 300 es\n"
        "precision highp float;\n"
        "in highp vec4 a_position;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = a_position;\n"
        "}\n";

    const std::string &fragShader =
        "#version 300 es\n"
        "precision highp float;\n"
        "out highp vec4 my_color;\n"
        "struct S\n"
        "{\n"
        "    mat2x4 m;\n"
        "    bool b;\n"
        "};\n"
        "uniform S uni;\n"
        "void main()\n"
        "{\n"
        "    my_color = vec4(1.0);\n"
        "    if (!uni.b) { my_color.g = 0.0; }"
        "}\n";

    ANGLE_GL_PROGRAM(program, vertShader, fragShader);

    glUseProgram(program.get());

    GLint location = glGetUniformLocation(program.get(), "uni.b");
    ASSERT_NE(-1, location);
    glUniform1i(location, 1);

    drawQuad(program.get(), "a_position", 0.0f);

    ASSERT_GL_NO_ERROR();
    EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::white);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_INSTANTIATE_TEST(UniformTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_D3D11_FL9_3(),
                       ES2_OPENGL(),
                       ES2_OPENGLES());
ANGLE_INSTANTIATE_TEST(UniformTestES3, ES3_D3D11(), ES3_OPENGL(), ES3_OPENGLES());
ANGLE_INSTANTIATE_TEST(UniformTestES31, ES31_D3D11(), ES31_OPENGL(), ES31_OPENGLES());

} // namespace
