#define GLFW_INCLUDE_NONE 1
#include <GLFW/glfw3.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include "glad/gl.h"

#define WIDTH 320
#define HEIGHT 240

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_THRES 0.90

#define STR1(x) #x
#define STR(x) STR1(x)

struct pass {
    unsigned int fb;
    unsigned int tex;
    unsigned int prog;
    unsigned int uthres;
    unsigned int utime;
    unsigned int ufb;
};

static void info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fputc(0x0A, stderr);
    va_end(ap);
}

static void *malloc_or_die(size_t n)
{
    void *result = malloc(n);

    if(!result) {
        info("malloc(%zu) failed", n);
        abort();
    }

    return result;
}

static unsigned int make_shader(unsigned int stage, const char *source)
{
    int status;
    char *infolog;
    unsigned int shader = glCreateShader(stage);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &status);
    if(status > 1) {
        infolog = malloc_or_die(status);
        glGetShaderInfoLog(shader, status, NULL, infolog);
        info("%s\n%s", source, infolog);
        free(infolog);
    }

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if(status == GL_FALSE) {
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

static unsigned int make_program(const char *vs, const char *fs)
{
    int status;
    char *infolog;
    unsigned int prog;
    unsigned int vert = make_shader(GL_VERTEX_SHADER, vs);
    unsigned int frag = make_shader(GL_FRAGMENT_SHADER, fs);

    if(vert && frag) {
        prog = glCreateProgram();
        glAttachShader(prog, vert);
        glAttachShader(prog, frag);
        glLinkProgram(prog);

        glDeleteShader(vert);
        glDeleteShader(frag);

        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &status);
        if(status > 1) {
            infolog = malloc_or_die(status);
            glGetProgramInfoLog(prog, status, NULL, infolog);
            info("<PROG>\n%s", infolog);
            free(infolog);
        }
        
        glGetProgramiv(prog, GL_LINK_STATUS, &status);

        if(status == GL_FALSE) {
            glDeleteProgram(prog);
            return 0;
        }

        return prog;
    }

    if(vert)
        glDeleteShader(vert);
    if(frag)
        glDeleteShader(frag);
    return 0;
}

static void init_pass(struct pass *p, const char *vs, const char *fs)
{
    glGenTextures(1, &p->tex);
    glBindTexture(GL_TEXTURE_2D, p->tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, &p->fb);
    glBindFramebuffer(GL_FRAMEBUFFER, p->fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, p->tex, 0);

    p->prog = make_program(vs, fs);

    if(!p->prog) {
        info("pass: create failed");
        abort();
    }

    p->uthres = glGetUniformLocation(p->prog, "thres");
    p->utime = glGetUniformLocation(p->prog, "curtime");
    p->ufb = glGetUniformLocation(p->prog, "pfb");
}

static void deinit_pass(struct pass *p)
{
    glDeleteProgram(p->prog);
    glDeleteFramebuffers(1, &p->fb);
    glDeleteTextures(1, &p->tex);
}

static const char *pass_vs =
    "#version 330 core\n"
    "out vec2 uv;"
    "const vec2 verts[8] = vec2[8](\n"
    "vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, -1.0),\n"
    "vec2(1.0, 1.0), vec2(-1.0, 1.0), vec2(-1.0, -1.0), vec2(-1.0, 1.0));\n"
    "void main(void){\n"
    "uv = 0.5 * (vec2(1.0, 1.0) + verts[gl_VertexID]);\n"
    "gl_Position = vec4(verts[gl_VertexID], vec2(0.0, 1.0));}\n";

static const char *pass1_fs =
    "#version 330 core\n"
    "in vec2 uv;\n"
    "layout(location = 0) out vec4 target;\n"
    "uniform float thres\n;"
    "uniform float curtime;\n"
    "float rand(vec3 v) { float r = dot(sin(v), vec3(12.9898, 78.233, 37.719)); return fract(sin(r) * 143758.5453); }\n"
    "void main(void) {\n"
    "float cutoff = smoothstep(0.98, 0.97, uv.x);\n"
    "float noise = step(clamp(thres, 0.01, 0.99), rand(vec3(100.0 * uv, curtime)));"
    "float color = 5.0 * cutoff * noise;\n"
    "target = vec4(vec3(color), 1.0);}\n";

static const char *pass2_fs =
    "#version 330 core\n"
    "in vec2 uv;\n"
    "layout(location = 0) out vec4 target;\n"
    "uniform float curtime;\n"
    "uniform sampler2D pfb;\n"
    "void main(void) {\n"
    "const int steps = 16;\n"
    "const float fsteps = float(steps);\n"
    "float st = 1.0 / float(" STR(WIDTH) ");\n"
    "target = vec4(0.0, 0.0, 0.0, 0.0);\n"
    "for(int i = 1; i <= steps; ++i)\n"
    "target += texture(pfb, uv - vec2(float(i) * st, 0.0)) / float(i) * 16.0;\n"
    "target += texture(pfb, uv);\n"
    "target /= fsteps;}\n";

int main(void)
{
    float curtime;
    GLFWwindow *window;
    struct pass passes[2];
    unsigned int vao;
    int width, height;
    double mx, my;
    double thres = DEFAULT_THRES;

    if(!glfwInit()) {
        info("glfw: init failed");
        abort();
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    window = glfwCreateWindow(DEFAULT_WIDTH, DEFAULT_HEIGHT, __FILE__, NULL, NULL);   
    if(!window) {
        info("glfw: create window failed");
        abort();
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if(!gladLoadGL(&glfwGetProcAddress)) {
        info("glad: unable to load function pointers");
        abort();
    }

    /* Setup pass 1 */
    info("initializing pass 1");
    init_pass(&passes[0], pass_vs, pass1_fs);
    init_pass(&passes[1], pass_vs, pass2_fs);

    glGenVertexArrays(1, &vao);

    while(!glfwWindowShouldClose(window)) {
        curtime = glfwGetTime() / 1000.0;
        glfwGetFramebufferSize(window, &width, &height);
        glfwGetCursorPos(window, &mx, &my);
        mx /= width;
        my /= height;

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            /* Update threshold */
            thres = 1.0 - mx;
        }

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            /* Reset threshold */
            thres = DEFAULT_THRES;
        }

        /* Setup environment */
        glBindVertexArray(vao);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        /* Render pass 1 */
        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, passes[0].fb);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(passes[0].prog);
        glUniform1f(passes[0].uthres, thres);
        glUniform1f(passes[0].utime, curtime);
        glDrawArrays(GL_TRIANGLES, 0, 8);

        /* Render pass 2 */
        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, passes[1].fb);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(passes[1].prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, passes[0].tex);
        glUniform1f(passes[1].utime, curtime);
        glUniform1i(passes[1].ufb, 0);
        glDrawArrays(GL_TRIANGLES, 0, 8);

        /* Blit to screen */
        glViewport(0, 0, width, height);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, passes[1].fb);
        glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    deinit_pass(&passes[1]);
    deinit_pass(&passes[0]);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
