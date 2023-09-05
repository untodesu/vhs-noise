#define GLFW_INCLUDE_NONE 1
#include <GLFW/glfw3.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "glad/gl.h"

#define WIDTH 320
#define HEIGHT 240

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480
#define DEFAULT_PARAM1 0.95
#define DEFAULT_PARAM2 0.00

#define STR1(x) #x
#define STR(x) STR1(x)

struct pass {
    unsigned int fb;
    unsigned int tex;
    unsigned int prog;
    unsigned int u_size;
    unsigned int u_time;
    unsigned int u_params;
    unsigned int u_pass;
};

struct timings {
    float curtime;
    float prevtime;
    float slowtime;
    float frametime;
    float frametime_avg;
    float perftime;
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

static char *malloc_file(const char *path)
{
    size_t length;
    FILE *file = fopen(path, "r");
    char *buffer;

    if(file) {
        fseek(file, 0, SEEK_END);
        length = ftell(file) + 1;
        fseek(file, 0, SEEK_SET);
        buffer = malloc_or_die(length);
        memset(buffer, 0, length);
        fread(buffer, 1, length - 1, file);
        fclose(file);
        return buffer;
    }

    return NULL;
}

static unsigned int make_shader(unsigned int stage, const char *path)
{
    int status;
    char *infolog;
    char *source = malloc_file(path);
    unsigned int shader;

    if(source) {
        shader = glCreateShader(stage);
        glShaderSource(shader, 1, ((const GLchar **)&source), NULL);
        glCompileShader(shader);

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &status);
        if(status > 1) {
            infolog = malloc_or_die(status);
            glGetShaderInfoLog(shader, status, NULL, infolog);
            info("%s\n%s", source, infolog);
            free(infolog);
        }

        free(source);

        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if(status == GL_FALSE) {
            glDeleteShader(shader);
            return 0;
        }

        return shader;
    }

    return 0;
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

    /* Fetch uniform locations */
    p->u_size = glGetUniformLocation(p->prog, "size");
    p->u_time = glGetUniformLocation(p->prog, "time");
    p->u_params = glGetUniformLocation(p->prog, "params");
    p->u_pass = glGetUniformLocation(p->prog, "pass");
}

static void deinit_pass(struct pass *p)
{
    glDeleteProgram(p->prog);
    glDeleteFramebuffers(1, &p->fb);
    glDeleteTextures(1, &p->tex);
}

static void on_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    /* unused */ (void)scancode;
    /* unused */ (void)mods;

    if(action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        return;
    }
}

int main(void)
{
    struct timings t;
    GLFWwindow *window;
    unsigned int vaobj;
    struct pass passes[2];
    double params[2];
    double mouse[2];
    int size[2];

    /* Default params */
    params[0] = DEFAULT_PARAM1;
    params[1] = DEFAULT_PARAM2;

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

    glfwSetKeyCallback(window, &on_key);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if(!gladLoadGL(&glfwGetProcAddress)) {
        info("glad: unable to load function pointers");
        abort();
    }

    glGenVertexArrays(1, &vaobj);

    init_pass(&passes[0], "pass_v.vert", "pass_1.frag");
    init_pass(&passes[1], "pass_v.vert", "pass_2.frag");

    t.curtime = 0.0f;
    t.prevtime = glfwGetTime();
    t.slowtime = 0.0f;
    t.frametime = 0.0f;
    t.frametime_avg = 0.0f;
    t.perftime = 0.0f;

    while(!glfwWindowShouldClose(window)) {
        t.curtime = glfwGetTime();
        t.slowtime = t.curtime * 0.001f;
        t.frametime = (t.curtime - t.prevtime);
        t.prevtime = t.curtime;
        t.frametime_avg += t.frametime;
        t.frametime_avg *= 0.5f;

        if(t.curtime >= t.perftime) {
            info("%.04fms / %.03f FPS", 1000.0f * t.frametime_avg, 1.0f / t.frametime_avg);
            t.perftime = t.curtime + 1.0f;
        }

        glfwGetFramebufferSize(window, &size[0], &size[1]);
        glfwGetCursorPos(window, &mouse[0], &mouse[1]);

        /* Mouse positions are relative */
        mouse[0] /= size[0];
        mouse[1] /= size[1];

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            /* Overall noise factor */
            params[0] = 1.0 - mouse[0];
        }

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
            /* Bottom VHS-like noise */
            params[1] = mouse[0];
        }

        if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
            params[0] = DEFAULT_PARAM1;
            params[1] = DEFAULT_PARAM2;
        }

        /* Setup environment */
        glBindVertexArray(vaobj);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

        /* Render pass 1 */
        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, passes[0].fb);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(passes[0].prog);
        glUniform2f(passes[0].u_size, WIDTH, HEIGHT);
        glUniform2f(passes[0].u_time, t.curtime, t.slowtime);
        glUniform2f(passes[0].u_params, params[0], params[1]);
        glUniform1i(passes[0].u_pass, 0); /* unused */
        glDrawArrays(GL_TRIANGLES, 0, 8);

        /* Render pass 2 */
        glViewport(0, 0, WIDTH, HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, passes[1].fb);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(passes[1].prog);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, passes[0].tex);
        glUniform2f(passes[1].u_size, WIDTH, HEIGHT);
        glUniform2f(passes[1].u_time, t.curtime, t.slowtime);
        glUniform2f(passes[1].u_params, params[0], params[1]);
        glUniform1i(passes[1].u_pass, 0); /* GL_TEXTURE0 */
        glDrawArrays(GL_TRIANGLES, 0, 8);

        /* Blit to screen */
        glViewport(0, 0, size[0], size[1]);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, passes[1].fb);
        glBlitFramebuffer(0, 0, WIDTH, HEIGHT, 0, 0, size[0], size[1], GL_COLOR_BUFFER_BIT, GL_LINEAR);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &vaobj);

    deinit_pass(&passes[1]);
    deinit_pass(&passes[0]);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
