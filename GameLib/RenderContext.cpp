/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-02-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include "GameException.h"

#include <cstring>

RenderContext::RenderContext(
    ResourceLoader & resourceLoader,
    ProgressCallback const & progressCallback)
    : // Clouds
      mCloudShaderProgram()
    , mCloudShaderAmbientLightIntensityParameter(0)
    , mCloudBuffer()
    , mCloudBufferSize(0u)
    , mCloudBufferMaxSize(0u)    
    , mCloudVBO()
    , mCloudTextureDatas()
    , mCloudTextures()
    // Land
    , mLandShaderProgram()
    , mLandShaderAmbientLightIntensityParameter(0)
    , mLandShaderOrthoMatrixParameter(0)
    , mLandBuffer()
    , mLandBufferSize(0u)
    , mLandBufferMaxSize(0u)
    , mLandVBO()
    , mLandTexture()
    , mLandTextureData()
    // Water
    , mWaterShaderProgram()
    , mWaterShaderAmbientLightIntensityParameter(0)
    , mWaterShaderWaterTransparencyParameter(0)
    , mWaterShaderOrthoMatrixParameter(0)
    , mWaterBuffer()
    , mWaterBufferSize(0u)
    , mWaterBufferMaxSize(0u)
    , mWaterVBO()
    , mWaterTexture()
    , mWaterTextureData()
    // Multi-purpose shaders
    , mMatteNdcShaderProgram()
    , mMatteNdcShaderColorParameter(0)
    , mMatteNdcVBO()
    , mMatteWorldShaderProgram()
    , mMatteWorldShaderColorParameter(0)
    , mMatteWorldShaderOrthoMatrixParameter(0)
    , mMatteWorldVBO()
    // Render parameters
    , mZoom(1.0f)
    , mCamX(0.0f)
    , mCamY(0.0f)
    , mCanvasWidth(100)
    , mCanvasHeight(100)
    , mAmbientLightIntensity(1.0f)
    , mWaterTransparency(0.66f)
    , mShowStress(false)
    , mUseXRayMode(false)
    , mShowShipThroughWater(false)
    , mDrawPointsOnly(false)
{
    GLuint tmpGLuint;

    //
    // Init OpenGL
    //

    GameOpenGL::InitOpenGL();

    // Activate texture unit 0
    glActiveTexture(GL_TEXTURE0);


    //
    // Load textures
    //

    mCloudTextureDatas = resourceLoader.LoadTexturesRgba(
        "cloud",
        [&progressCallback](float progress, std::string const &)
        {
            if (progressCallback)
                progressCallback(progress / 3.0f, "Loading textures...");
        });

    if (progressCallback)
        progressCallback(0.666f, "Loading textures...");

    mLandTextureData.emplace(resourceLoader.LoadTextureRgb(std::string("sand_1.jpg")));

    if (progressCallback)
        progressCallback(1.0f, "Loading textures...");

    mWaterTextureData.emplace(resourceLoader.LoadTextureRgb(std::string("water_1.jpg")));

    // Generate all texture names
    size_t const numberOfTextures = mCloudTextureDatas.size() + 2U;
    std::vector<GLuint> textureNames(numberOfTextures, 0U);
    glGenTextures(static_cast<GLsizei>(numberOfTextures), textureNames.data());



    //
    // Clouds 
    //

    // Create program

    mCloudShaderProgram = glCreateProgram();

    char const * cloudVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute vec2 inputTexturePos;
        
        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = vec4(inputPos.xy, -1.0, 1.0);
            texturePos = inputTexturePos;
        }
    )";

    GameOpenGL::CompileShader(cloudVertexShaderSource, GL_VERTEX_SHADER, mCloudShaderProgram);

    char const * cloudFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;

        void main()
        {
            gl_FragColor = texture2D(inputTexture, texturePos) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(cloudFragmentShaderSource, GL_FRAGMENT_SHADER, mCloudShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mCloudShaderProgram, 0, "inputPos");
    glBindAttribLocation(*mCloudShaderProgram, 1, "inputTexturePos");

    // Link
    GameOpenGL::LinkShaderProgram(mCloudShaderProgram, "Cloud");

    // Get uniform locations
    mCloudShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mCloudShaderProgram, "paramAmbientLightIntensity");

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mCloudVBO = tmpGLuint;

    // Set hardcoded parameters
    glUseProgram(*mCloudShaderProgram);
    glUseProgram(0);

    // Create textures
    for (size_t i = 0; i < mCloudTextureDatas.size(); ++i)
    {
        // Create texture
        mCloudTextures.emplace_back(textureNames.back());
        textureNames.pop_back();

        glBindTexture(GL_TEXTURE_2D, *mCloudTextures.back());

        // Set repeat mode
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Set texture filtering parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Upload texture data
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mCloudTextureDatas[i].Width, mCloudTextureDatas[i].Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, mCloudTextureDatas[i].Data.get());
        if (GL_NO_ERROR != glGetError())
        {
            throw GameException("Error uploading cloud texture onto GPU");
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    assert(mCloudTextureDatas.size() == mCloudTextures.size());

    //
    // Land 
    //

    // Create program

    mLandShaderProgram = glCreateProgram();

    char const * landVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        
        // Parameters
        uniform mat4 paramOrthoMatrix;

        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
            texturePos = inputPos.xy;
        }
    )";

    GameOpenGL::CompileShader(landVertexShaderSource, GL_VERTEX_SHADER, mLandShaderProgram);

    char const * landFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;
        uniform vec2 paramTextureScaling;

        void main()
        {
            gl_FragColor = texture2D(inputTexture, texturePos * paramTextureScaling) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(landFragmentShaderSource, GL_FRAGMENT_SHADER, mLandShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mLandShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mLandShaderProgram, "Land");

    // Get uniform locations
    mLandShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramAmbientLightIntensity");
    GLint landShaderTextureScalingParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramTextureScaling");
    mLandShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mLandShaderProgram, "paramOrthoMatrix");

    // Create VBO    
    glGenBuffers(1, &tmpGLuint);
    mLandVBO = tmpGLuint;

    // Calculate scaling: we want a tile to be repeated every these many world units
    static constexpr float LandTileWorldSize = 128.0f;
    
    // Set hardcoded parameters
    glUseProgram(*mLandShaderProgram);
    glUniform2f(
        landShaderTextureScalingParameter, 
        1.0f / LandTileWorldSize,
        1.0f / LandTileWorldSize);
    glUseProgram(0);

    // Create texture
    mLandTexture = textureNames.back();
    textureNames.pop_back(); 

    glBindTexture(GL_TEXTURE_2D, *mLandTexture);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mLandTextureData->Width, mLandTextureData->Height, 0, GL_RGB, GL_UNSIGNED_BYTE, mLandTextureData->Data.get());
    if (GL_NO_ERROR != glGetError())
    {
        throw GameException("Error uploading land texture onto GPU");
    }

    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Water 
    //

    // Create program

    mWaterShaderProgram = glCreateProgram();

    char const * waterVertexShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;
        attribute float inputTextureY;

        // Parameters
        uniform mat4 paramOrthoMatrix;

        // Outputs
        varying vec2 texturePos;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
            texturePos = vec2(inputPos.x, inputTextureY);
        }
    )";

    GameOpenGL::CompileShader(waterVertexShaderSource, GL_VERTEX_SHADER, mWaterShaderProgram);

    char const * waterFragmentShaderSource = R"(

        // Inputs from previous shader
        varying vec2 texturePos;

        // The texture
        uniform sampler2D inputTexture;

        // Parameters        
        uniform float paramAmbientLightIntensity;
        uniform float paramWaterTransparency;
        uniform vec2 paramTextureScaling;

        void main()
        {
            vec4 textureColor = texture2D(inputTexture, texturePos * paramTextureScaling);
            gl_FragColor = vec4(textureColor.xyz, 1.0 - paramWaterTransparency) * paramAmbientLightIntensity;
        } 
    )";

    GameOpenGL::CompileShader(waterFragmentShaderSource, GL_FRAGMENT_SHADER, mWaterShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mWaterShaderProgram, 0, "inputPos");
    glBindAttribLocation(*mWaterShaderProgram, 1, "inputTextureY");

    // Link
    GameOpenGL::LinkShaderProgram(mWaterShaderProgram, "Water");

    // Get uniform locations    
    mWaterShaderAmbientLightIntensityParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramAmbientLightIntensity");
    mWaterShaderWaterTransparencyParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramWaterTransparency");
    GLint waterShaderTextureScalingParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramTextureScaling");
    mWaterShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mWaterShaderProgram, "paramOrthoMatrix");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mWaterVBO = tmpGLuint;

    // Calculate scaling: we want a tile to be repeated every these many world units
    static constexpr float WaterTileWorldSize = 128.0f;

    // Set hardcoded parameters
    glUseProgram(*mWaterShaderProgram);
    glUniform2f(
        waterShaderTextureScalingParameter,
        1.0f / WaterTileWorldSize,
        1.0f / WaterTileWorldSize);
    glUseProgram(0);

    // Create texture
    mWaterTexture = textureNames.back();
    textureNames.pop_back();

    glBindTexture(GL_TEXTURE_2D, *mWaterTexture);

    // Set repeat mode
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload texture data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, mWaterTextureData->Width, mWaterTextureData->Height, 0, GL_RGB, GL_UNSIGNED_BYTE, mWaterTextureData->Data.get());
    if (GL_NO_ERROR != glGetError())
    {
        throw GameException("Error uploading water texture onto GPU");
    }

    glBindTexture(GL_TEXTURE_2D, 0);


    //
    // Multi-purpose Matte NDC shader
    //

    mMatteNdcShaderProgram = glCreateProgram();

    char const * matteNdcShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;

        void main()
        {
            gl_Position = vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(matteNdcShaderSource, GL_VERTEX_SHADER, mMatteNdcShaderProgram);

    char const * matteNdcFragmentShaderSource = R"(

        // Params
        uniform vec4 paramCol;

        void main()
        {
            gl_FragColor = paramCol;
        } 
    )";

    GameOpenGL::CompileShader(matteNdcFragmentShaderSource, GL_FRAGMENT_SHADER, mMatteNdcShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mMatteNdcShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mMatteNdcShaderProgram, "Matte NDC");

    // Get uniform locations
    mMatteNdcShaderColorParameter = GameOpenGL::GetParameterLocation(mMatteNdcShaderProgram, "paramCol");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mMatteNdcVBO = tmpGLuint;


    //
    // Multi-purpose Matte World shader
    //

    mMatteWorldShaderProgram = glCreateProgram();

    char const * matteWorldShaderSource = R"(

        // Inputs
        attribute vec2 inputPos;

        // Params
        uniform mat4 paramOrthoMatrix;

        void main()
        {
            gl_Position = paramOrthoMatrix * vec4(inputPos.xy, -1.0, 1.0);
        }
    )";

    GameOpenGL::CompileShader(matteWorldShaderSource, GL_VERTEX_SHADER, mMatteWorldShaderProgram);

    char const * matteWorldFragmentShaderSource = R"(

        // Params
        uniform vec4 paramCol;

        void main()
        {
            gl_FragColor = paramCol;
        } 
    )";

    GameOpenGL::CompileShader(matteWorldFragmentShaderSource, GL_FRAGMENT_SHADER, mMatteWorldShaderProgram);

    // Bind attribute locations
    glBindAttribLocation(*mMatteWorldShaderProgram, 0, "inputPos");

    // Link
    GameOpenGL::LinkShaderProgram(mMatteWorldShaderProgram, "Matte World");

    // Get uniform locations
    mMatteWorldShaderColorParameter = GameOpenGL::GetParameterLocation(mMatteWorldShaderProgram, "paramCol");
    mMatteWorldShaderOrthoMatrixParameter = GameOpenGL::GetParameterLocation(mMatteWorldShaderProgram, "paramOrthoMatrix");

    // Create VBO
    glGenBuffers(1, &tmpGLuint);
    mMatteWorldVBO = tmpGLuint;


    //
    // Initialize ortho matrix
    //

    for (size_t r = 0; r < 4; ++r)
    {
        for (size_t c = 0; c < 4; ++c)
        {
            mOrthoMatrix[r][c] = 0.0f;
        }
    }

    assert(textureNames.empty());
}

RenderContext::~RenderContext()
{
    glUseProgram(0u);
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::Reset()
{
    // Clear ships
    mShips.clear();

    // TODO: might also reset all the other render contextes, once we have refactored
    // them out and stored them as uq_ptr's
}

void RenderContext::AddShip(
    int shipId,
    std::optional<ImageData> const & texture)
{
    // Add the ship
    assert(shipId == mShips.size());
    mShips.emplace_back(new ShipRenderContext(texture));
}

//////////////////////////////////////////////////////////////////////////////////

void RenderContext::RenderStart()
{
    // Set anti-aliasing for lines and polygons
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH, GL_NICEST);

    // Set nearest interpolation for textures
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Enable stencil test    
    glEnable(GL_STENCIL_TEST);    

    // Clear canvas 
    static const vec3f ClearColorBase(0.529f, 0.808f, 0.980f); // (cornflower blue)
    vec3f clearColor = ClearColorBase * mAmbientLightIntensity;
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glStencilMask(0x00);
}

void RenderContext::RenderCloudsStart(size_t clouds)
{
    if (clouds != mCloudBufferMaxSize)
    {
        // Realloc
        mCloudBuffer.reset();
        mCloudBuffer.reset(new CloudElement[clouds]);
        mCloudBufferMaxSize = clouds;
    }

    mCloudBufferSize = 0u;
}

void RenderContext::RenderCloudsEnd()
{
    //
    // Draw stencil
    //

    // Use matte world program
    glUseProgram(*mMatteWorldShaderProgram);

    // Set parameters
    glUniform4f(mMatteWorldShaderColorParameter, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniformMatrix4fv(mMatteWorldShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Bind water buffer
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Disable writing to the color buffer
    glColorMask(false, false, false, false);

    // Write all one's to stencil buffer
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilMask(0xFF);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));

    // Don't write anything to stencil buffer now
    glStencilMask(0x00);

    // Re-enable writing to the color buffer
    glColorMask(true, true, true, true);
    
    // Stop using program
    glUseProgram(0);


    //
    // Draw clouds
    //

    assert(mCloudBufferSize == mCloudBufferMaxSize);

    // Use program
    glUseProgram(*mCloudShaderProgram);

    // Set parameters
    glUniform1f(mCloudShaderAmbientLightIntensityParameter, mAmbientLightIntensity);

    // Upload cloud buffer 
    glBindBuffer(GL_ARRAY_BUFFER, *mCloudVBO);
    glBufferData(GL_ARRAY_BUFFER, mCloudBufferSize * sizeof(CloudElement), mCloudBuffer.get(), GL_DYNAMIC_DRAW);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Describe InputTexturePos
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (2 + 2) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable stenciling - only draw where there are no 1's
    glStencilFunc(GL_NOTEQUAL, 1, 0xFF);

    // Draw
    for (size_t c = 0; c < mCloudBufferSize; ++c)
    {
        // Bind Texture
        glBindTexture(GL_TEXTURE_2D, *(mCloudTextures[GetCloudTextureIndex(c)]));

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, static_cast<GLint>(4 * c), 4);
    }

    // Disable stenciling - draw always
    glStencilFunc(GL_ALWAYS, 0, 0x00);

    // Stop using program
    glUseProgram(0);
}

void RenderContext::UploadLandAndWaterStart(size_t slices)
{
    //
    // Prepare land buffer
    //

    if (slices + 1 != mLandBufferMaxSize)
    {
        // Realloc
        mLandBuffer.reset();
        mLandBuffer.reset(new LandElement[slices + 1]);
        mLandBufferMaxSize = slices + 1;
    }

    mLandBufferSize = 0u;

    //
    // Prepare water buffer
    //

    if (slices + 1 != mWaterBufferMaxSize)
    {
        // Realloc
        mWaterBuffer.reset();
        mWaterBuffer.reset(new WaterElement[slices + 1]);
        mWaterBufferMaxSize = slices + 1;
    }

    mWaterBufferSize = 0u;
}

void RenderContext::UploadLandAndWaterEnd()
{
    //
    // Upload land buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);
    glBufferData(GL_ARRAY_BUFFER, mLandBufferSize * sizeof(LandElement), mLandBuffer.get(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0u);

    //
    // Upload water buffer
    //

    assert(mWaterBufferSize == mWaterBufferMaxSize);

    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);
    glBufferData(GL_ARRAY_BUFFER, mWaterBufferSize * sizeof(WaterElement), mWaterBuffer.get(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0u);
}

void RenderContext::RenderLand()
{
    // Use program
    glUseProgram(*mLandShaderProgram);

    // Set parameters
    glUniform1f(mLandShaderAmbientLightIntensityParameter, mAmbientLightIntensity);
    glUniformMatrix4fv(mLandShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Bind Texture
    glBindTexture(GL_TEXTURE_2D, *mLandTexture);

    // Bind land buffer
    glBindBuffer(GL_ARRAY_BUFFER, *mLandVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mLandBufferSize));

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderWater()
{
    // Use program
    glUseProgram(*mWaterShaderProgram);

    // Set parameters
    glUniform1f(mWaterShaderAmbientLightIntensityParameter, mAmbientLightIntensity);
    glUniform1f(mWaterShaderWaterTransparencyParameter, mWaterTransparency);
    glUniformMatrix4fv(mWaterShaderOrthoMatrixParameter, 1, GL_FALSE, &(mOrthoMatrix[0][0]));

    // Bind Texture
    glBindTexture(GL_TEXTURE_2D, *mWaterTexture);

    // Bind water buffer
    glBindBuffer(GL_ARRAY_BUFFER, *mWaterVBO);

    // Describe InputPos
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Describe InputTextureY
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, (2 + 1) * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Enable blend (to make water transparent)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, static_cast<GLsizei>(2 * mWaterBufferSize));

    // Stop using program
    glUseProgram(0);
}

void RenderContext::RenderEnd()
{
    glFlush();
}

////////////////////////////////////////////////////////////////////////////////////

void RenderContext::CalculateOrthoMatrix()
{
    static constexpr float zFar = 1000.0f;
    static constexpr float zNear = 1.0f;

    mOrthoMatrix[0][0] = 2.0f / mVisibleWorldWidth;
    mOrthoMatrix[1][1] = 2.0f / mVisibleWorldHeight;
    mOrthoMatrix[2][2] = -2.0f / (zFar - zNear);
    mOrthoMatrix[3][0] = -2.0f * mCamX / mVisibleWorldWidth;
    mOrthoMatrix[3][1] = -2.0f * mCamY / mVisibleWorldHeight;
    mOrthoMatrix[3][2] = -(zFar + zNear) / (zFar - zNear);
    mOrthoMatrix[3][3] = 1.0f;
}

void RenderContext::CalculateVisibleWorldCoordinates()
{
    mVisibleWorldHeight = 2.0f * 70.0f / (mZoom + 0.001f);
    mVisibleWorldWidth = static_cast<float>(mCanvasWidth) / static_cast<float>(mCanvasHeight) * mVisibleWorldHeight;
}