// Include the BGFX header file
#include <bgfx/bgfx.h>

// Define the vertex structure for the circle
struct CircleVertex
{
    float x, y, z; // Position
    uint32_t abgr; // Color
};

// Define the vertex declaration for the circle
static bgfx::VertexDecl s_circleDecl;

// Initialize the vertex declaration
void initCircleDecl()
{
    s_circleDecl.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
}

// Create a transient vertex buffer for the circle
bgfx::TransientVertexBuffer createCircleVB(float radius, uint32_t color, uint16_t segments)
{
    // Calculate the number of vertices and the size of the buffer
    uint16_t numVertices = segments + 1;
    uint32_t size = numVertices * sizeof(CircleVertex);

    // Allocate a transient vertex buffer
    bgfx::TransientVertexBuffer tvb;
    bgfx::allocTransientVertexBuffer(&tvb, numVertices, s_circleDecl);

    // Get a pointer to the vertex data
    CircleVertex* vertex = (CircleVertex*)tvb.data;

    // Add the center vertex
    vertex->x = 0.0f;
    vertex->y = 0.0f;
    vertex->z = 0.0f;
    vertex->abgr = color;
    vertex++;

    // Add the perimeter vertices
    float angleStep = 2.0f * M_PI / segments;
    float angle = 0.0f;
    for (uint16_t i = 0; i < segments; i++)
    {
        vertex->x = radius * cosf(angle);
        vertex->y = radius * sinf(angle);
        vertex->z = 0.0f;
        vertex->abgr = color;
        vertex++;
        angle += angleStep;
    }

    // Return the transient vertex buffer
    return tvb;
}

// Create a transient index buffer for the circle
bgfx::TransientIndexBuffer createCircleIB(uint16_t segments)
{
    // Calculate the number of indices and the size of the buffer
    uint16_t numIndices = segments * 3;
    uint32_t size = numIndices * sizeof(uint16_t);

    // Allocate a transient index buffer
    bgfx::TransientIndexBuffer tib;
    bgfx::allocTransientIndexBuffer(&tib, numIndices);

    // Get a pointer to the index data
    uint16_t* index = (uint16_t*)tib.data;

    // Add the triangle indices
    for (uint16_t i = 0; i < segments; i++)
    {
        index[0] = 0; // Center vertex
        index[1] = i + 1; // Current perimeter vertex
        index[2] = i + 2; // Next perimeter vertex
        index += 3;
    }

    // Fix the last triangle
    index[-1] = 1;

    // Return the transient index buffer
    return tib;
}

// Create a shader program for the circle
bgfx::ProgramHandle createCircleProgram()
{
    // Load the vertex shader and fragment shader binaries
    // These can be generated using the shaderc tool from BGFX
    bgfx::ShaderHandle vsh = bgfx::createShader(bgfx::makeRef(vshader_circle_bin, sizeof(vshader_circle_bin)));
    bgfx::ShaderHandle fsh = bgfx::createShader(bgfx::makeRef(fshader_circle_bin, sizeof(fshader_circle_bin)));

    // Create and return the shader program
    return bgfx::createProgram(vsh, fsh, true);
}

// Create a uniform variable for the circle color
bgfx::UniformHandle u_color = bgfx::createUniform("u_color", bgfx::UniformType::Vec4);

// Draw a filled circle with the given parameters
void drawCircle(float x, float y, float radius, uint32_t color, uint16_t segments, bgfx::ProgramHandle program, const float* view, const float* proj)
{
    // Create a transient vertex buffer and index buffer for the circle
    bgfx::TransientVertexBuffer tvb = createCircleVB(radius, color, segments);
    bgfx::TransientIndexBuffer tib = createCircleIB(segments);

    // Set the vertex buffer and index buffer
    bgfx::setVertexBuffer(0, &tvb);
    bgfx::setIndexBuffer(&tib);

    // Set the shader program
    bgfx::setProgram(program);

    // Set the uniform variable for the circle color
    // The color is given in RGBA format, but BGFX uses ABGR internally
    float col[4];
    col[0] = (color >> 24) & 0xff;
    col[1] = (color >> 16) & 0xff;
    col[2] = (color >> 8) & 0xff;
    col[3] = color & 0xff;
    bgfx::setUniform(u_color, col);

    // Set the model matrix for the circle position
    float mtx[16];
    bx::mtxTranslate(mtx, x, y, 0.0f);
    bgfx::setTransform(mtx);

    // Set the view matrix and projection matrix
    bgfx::setViewTransform(0, view, proj);

    // Set the render state
    bgfx::setState(BGFX_STATE_DEFAULT);

    // Submit the draw call
    bgfx::submit(0);
}
