#pragma once


glm::vec3 DoNothing(triangle* tri, int depth, glm::vec3 p, glm::vec3 dir)
{
    return vec3(0);
}

glm::vec3 Shade(triangle* tri, int depth, glm::vec3 p, glm::vec3 dir)
{
    // Calculate ambient light (0.1 as specified in the assessment)
    float ambient = 0.1f;
    vec3 col = tri->v1.col * ambient;
    
    // Surface normal from triangle
    vec3 normal = tri->v1.nor;
    
    // Calculate light direction
    vec3 light_dir = normalize(light_pos - p);
    
    // Check if point is in shadow by shooting ray to light
    float shadow_t = FLT_MAX;
    vec3 shadow_col;
    trace(p, light_dir, shadow_t, shadow_col, depth + 1, DoNothing);
    
    // If not in shadow, add diffuse lighting using Lambert's Law
    if (shadow_t == FLT_MAX) {
        // Calculate diffuse contribution (max ensures it's non-negative)
        float diffuse = max(dot(normal, light_dir), 0.0f);
        col += tri->v1.col * diffuse;
    }
    
    // Calculate reflection (if needed and within recursion limit)
    if (tri->reflect && depth < max_recursion_depth) {
        // Calculate reflection direction using glm's reflect function
        vec3 reflection_dir = reflect(dir, normal);
        
        // Trace reflection ray
        float reflection_t = FLT_MAX;
        vec3 reflection_col;
        trace(p, reflection_dir, reflection_t, reflection_col, depth + 1, Shade);
        
        // Add reflection color
        col += reflection_col;
    }
    
    return col;
}

bool PointInTriangle(glm::vec3 pt, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
    // Based on Figure 6 in Ch12 - using cross products and dot products
    vec3 edge1 = v2 - v1;
    vec3 edge2 = v3 - v2;
    vec3 edge3 = v1 - v3;
    
    vec3 c1 = cross(edge1, pt - v1);
    vec3 c2 = cross(edge2, pt - v2);
    vec3 c3 = cross(edge3, pt - v3);
    
    // Calculate normal of the triangle
    vec3 normal = cross(edge1, v3 - v1);
    normal = normalize(normal);
    
    // Test if point is inside triangle using dot products
    float d1 = dot(normal, c1);
    float d2 = dot(normal, c2);
    float d3 = dot(normal, c3);
    
    // Point is inside if all dot products have the same sign (all positive or all negative)
    return (d1 >= 0 && d2 >= 0 && d3 >= 0) || (d1 <= 0 && d2 <= 0 && d3 <= 0);
}

float RayTriangleIntersection(glm::vec3 o, glm::vec3 dir, triangle* tri, glm::vec3& point)
{
    // Get triangle vertices
    const vec3& v1 = tri->v1.pos;
    const vec3& v2 = tri->v2.pos;
    const vec3& v3 = tri->v3.pos;
    
    // Calculate normal of triangle
    vec3 edge1 = v2 - v1;
    vec3 edge2 = v3 - v1;
    vec3 normal = cross(edge1, edge2);
    normal = normalize(normal);
    
    // Check if ray is parallel to plane
    float dot_nd = dot(normal, dir);
    if (abs(dot_nd) < 0.0001f) {
        return FLT_MAX; // Ray parallel to triangle
    }
    
    // Calculate t where ray intersects plane
    // Using the formula: t = ((p - o) · N) / (d · N) where p is a point on the plane
    float t = dot(normal, v1 - o) / dot_nd;
    
    // Ensure t is positive and not too small (to avoid self-intersection)
    if (t < 0.001f) {
        return FLT_MAX; // Intersection behind origin or too close
    }
    
    // Calculate intersection point
    point = o + dir * t;
    
    // Check if intersection point is inside triangle
    if (PointInTriangle(point, v1, v2, v3)) {
        return t;
    }
    
    return FLT_MAX; // Intersection not inside triangle
}

void trace(glm::vec3 o, glm::vec3 dir, float& t, glm::vec3& io_col, int depth, closest_hit p_hit)
{
    t = FLT_MAX;
    triangle* closest_triangle = nullptr;
    vec3 closest_point;
    
    // Find closest intersection
    for (int i = 0; i < tris.size(); i++) {
        vec3 intersection_point;
        float intersection_t = RayTriangleIntersection(o, dir, &tris[i], intersection_point);
        
        if (intersection_t < t) {
            t = intersection_t;
            closest_triangle = &tris[i];
            closest_point = intersection_point;
        }
    }
    
    // If we hit a triangle, shade it
    if (closest_triangle != nullptr) {
        io_col = p_hit(closest_triangle, depth, closest_point, dir);
    } else {
        io_col = bkgd; // Set background color if no intersection
    }
}

vec3 GetRayDirection(float px, float py, int W, int H, float aspect_ratio, float fov)
{
    // Using the formula from Chapter 12
    float f = tan(fov / 2.0f);
    
    // Camera basis vectors as specified in the assessment
    vec3 R(1.0f, 0.0f, 0.0f);
    vec3 U(0.0f, -1.0f, 0.0f); // Negative Y as specified
    vec3 F(0.0f, 0.0f, -1.0f);
    
    // Calculate normalized device coordinates
    float x_ndc = (2.0f * (px + 0.5f) / W) - 1.0f;
    float y_ndc = (2.0f * (py + 0.5f) / H) - 1.0f;
    
    // Calculate ray direction using the formula from Chapter 12
    vec3 d = aspect_ratio * f * x_ndc * R + f * y_ndc * U + F;
    d = normalize(d);
    
    return d;
}

void raytrace()
{
    // Calculate aspect ratio and field of view (90 degrees in radians)
    float aspect_ratio = (float)PIXEL_W / (float)PIXEL_H;
    float fov = 90.0f * (3.14159265358979323846f / 180.0f); // 90 degrees in radians
    
    for (int pixel_y = 0; pixel_y < PIXEL_H; ++pixel_y)
    {
        float percf = (float)pixel_y / (float)PIXEL_H;
        int perci = percf * 100;
        std::clog << "\rScanlines done: " << perci << "%" << ' ' << std::flush;

        for (int pixel_x = 0; pixel_x < PIXEL_W; ++pixel_x)
        {
            // Get ray direction for this pixel
            vec3 ray_dir = GetRayDirection(pixel_x, pixel_y, PIXEL_W, PIXEL_H, aspect_ratio, fov);
            
            // Trace ray and get color
            float t = FLT_MAX;
            vec3 pixel_color;
            trace(eye, ray_dir, t, pixel_color, 0, Shade);
            
            // Write color to buffer
            writeCol(pixel_color, pixel_x, pixel_y);
        }
    }
    std::clog << "\rFinish rendering.           \n";
}
