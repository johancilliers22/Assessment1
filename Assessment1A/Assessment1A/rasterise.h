#pragma once

#include <limits>
#include <iostream>
#include <string>

// Clear the color buffer with the specified color
void ClearColourBuffer(float col[4])
{
    // Set all pixels in the color buffer to the specified color
    for (int i = 0; i < PIXEL_W * PIXEL_H * 3; i += 3)
    {
        colour_buffer[i] = col[0] * 255.0f;     // R
        colour_buffer[i + 1] = col[1] * 255.0f; // G
        colour_buffer[i + 2] = col[2] * 255.0f; // B
    }
}

// Clear the depth buffer with maximum depth values
void ClearDepthBuffer()
{
    // Initialize depth buffer with maximum depth values
    for (int i = 0; i < PIXEL_W * PIXEL_H; i++)
    {
        depth_buffer[i] = std::numeric_limits<float>::max();
    }
}

// Apply transformation matrix to all vertices of all triangles
void ApplyTransformationMatrix(glm::mat4 T, vector<triangle> &tris)
{
    for (int i = 0; i < tris.size(); i++)
    {
        // Transform each vertex position by the transformation matrix
        tris[i].v1.pos = T * tris[i].v1.pos;
        tris[i].v2.pos = T * tris[i].v2.pos;
        tris[i].v3.pos = T * tris[i].v3.pos;

        // Transform normals with the transpose of the inverse of the upper-left 3x3 part of T
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(T)));
        tris[i].v1.nor = normalMat * tris[i].v1.nor;
        tris[i].v2.nor = normalMat * tris[i].v2.nor;
        tris[i].v3.nor = normalMat * tris[i].v3.nor;
    }
}

// Apply perspective division (divide x, y, z by w)
void ApplyPerspectiveDivision(vector<triangle> &tris)
{
    for (int i = 0; i < tris.size(); i++)
    {
        // Perspective division for v1
        if (tris[i].v1.pos.w != 0.0f)
        {
            tris[i].v1.pos.x /= tris[i].v1.pos.w;
            tris[i].v1.pos.y /= tris[i].v1.pos.w;
            tris[i].v1.pos.z /= tris[i].v1.pos.w;
        }

        // Perspective division for v2
        if (tris[i].v2.pos.w != 0.0f)
        {
            tris[i].v2.pos.x /= tris[i].v2.pos.w;
            tris[i].v2.pos.y /= tris[i].v2.pos.w;
            tris[i].v2.pos.z /= tris[i].v2.pos.w;
        }

        // Perspective division for v3
        if (tris[i].v3.pos.w != 0.0f)
        {
            tris[i].v3.pos.x /= tris[i].v3.pos.w;
            tris[i].v3.pos.y /= tris[i].v3.pos.w;
            tris[i].v3.pos.z /= tris[i].v3.pos.w;
        }
    }
}

// Apply viewport transformation to map NDC coordinates to screen coordinates
void ApplyViewportTransformation(int w, int h, vector<triangle> &tris)
{
    for (int i = 0; i < tris.size(); i++)
    {
        // Map x from [-1, 1] to [0, w]
        // Map y from [-1, 1] to [0, h]
        tris[i].v1.pos.x = (tris[i].v1.pos.x + 1.0f) * 0.5f * w;
        tris[i].v1.pos.y = (tris[i].v1.pos.y + 1.0f) * 0.5f * h;

        tris[i].v2.pos.x = (tris[i].v2.pos.x + 1.0f) * 0.5f * w;
        tris[i].v2.pos.y = (tris[i].v2.pos.y + 1.0f) * 0.5f * h;

        tris[i].v3.pos.x = (tris[i].v3.pos.x + 1.0f) * 0.5f * w;
        tris[i].v3.pos.y = (tris[i].v3.pos.y + 1.0f) * 0.5f * h;
    }
}

// Compute barycentric coordinates for point (px, py) within triangle t
void ComputeBarycentricCoordinates(int px, int py, triangle t, float &alpha, float &beta, float &gamma)
{
    // Get vertex positions
    float x1 = t.v1.pos.x;
    float y1 = t.v1.pos.y;
    float x2 = t.v2.pos.x;
    float y2 = t.v2.pos.y;
    float x3 = t.v3.pos.x;
    float y3 = t.v3.pos.y;

    // Calculate area of the full triangle using cross product
    float area = 0.5f * abs((x2 - x1) * (y3 - y1) - (x3 - x1) * (y2 - y1));

    // Handle degenerate triangles to avoid division by zero
    if (area < 0.00001f)
    {
        alpha = beta = gamma = 0.0f;
        return;
    }

    // Calculate barycentric coordinates using area method
    float area1 = 0.5f * abs((x2 - px) * (y3 - py) - (x3 - px) * (y2 - py));
    float area2 = 0.5f * abs((px - x1) * (y3 - y1) - (x3 - x1) * (py - y1));
    float area3 = 0.5f * abs((x2 - x1) * (py - y1) - (px - x1) * (y2 - y1));

    // Calculate alpha, beta, gamma
    alpha = area1 / area; // Weight of vertex 1
    beta = area2 / area;  // Weight of vertex 2
    gamma = area3 / area; // Weight of vertex 3
}

// Shade fragment using barycentric coordinates
void ShadeFragment(triangle tri, float &alpha, float &beta, float &gamma, glm::vec3 &col, float &depth)
{
    // Interpolate depth (z-value)
    depth = alpha * tri.v1.pos.z + beta * tri.v2.pos.z + gamma * tri.v3.pos.z;

    // Interpolate color
    col = alpha * tri.v1.col + beta * tri.v2.col + gamma * tri.v3.col;
}

// Determine if we're rendering a quad or cornell box based on triangle count and IDs
bool isQuad(const vector<triangle> &tris)
{
    // Quad typically has very few triangles (2 for a simple quad)
    if (tris.size() <= 2)
    {
        return true;
    }

    // Check the primID pattern - quads typically have simple primIDs (0,1)
    bool hasPrimID0or1 = false;
    bool hasHigherPrimIDs = false;

    for (const auto &tri : tris)
    {
        if (tri.primID <= 1)
        {
            hasPrimID0or1 = true;
        }
        else
        {
            hasHigherPrimIDs = true;
        }
    }

    // If we only have low primIDs and no higher ones, it's likely a quad
    if (hasPrimID0or1 && !hasHigherPrimIDs)
    {
        return true;
    }

    return false;
}

// Rasterize all triangles
void Rasterise(vector<triangle> tris)
{
    for (int py = 0; py < PIXEL_H; py++)
    {
        float percf = (float)py / (float)PIXEL_H;
        int perci = percf * 100;
        std::clog << "\rScanlines done: " << perci << "%" << ' ' << std::flush;

        for (int px = 0; px < PIXEL_W; px++)
        {
            // For each triangle, check if pixel is inside
            for (int i = 0; i < tris.size(); i++)
            {
                float alpha, beta, gamma;
                ComputeBarycentricCoordinates(px, py, tris[i], alpha, beta, gamma);

                // If point is inside triangle (all barycentric coordinates are between 0 and 1)
                if (alpha >= 0.0f && beta >= 0.0f && gamma >= 0.0f &&
                    alpha <= 1.0f && beta <= 1.0f && gamma <= 1.0f &&
                    (alpha + beta + gamma) <= 1.001f) // Small epsilon for floating point error
                {
                    // Get the shaded color and depth
                    glm::vec3 col;
                    float depth;
                    ShadeFragment(tris[i], alpha, beta, gamma, col, depth);

                    // Check depth buffer for this pixel
                    int buffer_index = py * PIXEL_W + px;
                    if (depth < depth_buffer[buffer_index])
                    {
                        // Update depth buffer and write color to framebuffer
                        depth_buffer[buffer_index] = depth;
                        // Flip the y-coordinate as mentioned in the troubleshooting section
                        writeColToDisplayBuffer(col, px, PIXEL_H - py - 1);
                    }
                }
            }
        }
    }
    std::clog << "\rFinish rendering.           \n";
}

// Main render function - implements the render loop from Chapter 3
void render(vector<triangle> &tris)
{
    // 1. Clear color and depth buffers
    float clear_color[4] = {0.0f, 0.0f, 0.0f, 1.0f}; // Black background
    ClearColourBuffer(clear_color);
    ClearDepthBuffer();

    // Detect whether we're rendering a quad or cornell box
    bool rendering_quad = isQuad(tris);

    // 2. Set up model-view-projection matrix
    glm::mat4 model = glm::mat4(1.0f); // Identity matrix initially
    glm::mat4 view;

    // Apply translations based on model type as specified in the assessment
    if (rendering_quad)
    {
        // Quad model (translation = (0, 0, -1))
        model = glm::translate(model, glm::vec3(0.0f, 0.0f, -1.0f));

        // For quad, set camera at origin looking along negative z-axis
        view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 0.0f),  // Camera position at origin
            glm::vec3(0.0f, 0.0f, -1.0f), // Looking along negative z-axis
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );
    }
    else
    {
        // Cornell box model (translation = (0.1, -2.5, -6))
        model = glm::translate(model, glm::vec3(0.1f, -2.5f, -6.0f));

        // For cornell box, use a camera position that gives a good view
        view = glm::lookAt(
            glm::vec3(0.0f, 0.0f, 0.0f),  // Camera position at origin
            glm::vec3(0.0f, 0.0f, -1.0f), // Looking along negative z-axis
            glm::vec3(0.0f, 1.0f, 0.0f)   // Up vector
        );
    }

    // Set up projection matrix with specified parameters
    float fovy = 60.0f;                             // Field of view: 60 degrees (as specified)
    float aspect = (float)PIXEL_W / (float)PIXEL_H; // Aspect ratio
    float zNear = 0.1f;                             // Near clipping plane (as specified)
    float zFar = 10.0f;                             // Far clipping plane (as specified)
    glm::mat4 projection = glm::perspective(glm::radians(fovy), aspect, zNear, zFar);

    // Combine matrices into model-view-projection matrix
    glm::mat4 mvp = projection * view * model;

    // 3. Apply transformations
    ApplyTransformationMatrix(mvp, tris);

    // 4. Apply perspective division
    ApplyPerspectiveDivision(tris);

    // 5. Apply viewport transformation
    ApplyViewportTransformation(PIXEL_W, PIXEL_H, tris);

    // 6. Rasterize triangles
    Rasterise(tris);
}