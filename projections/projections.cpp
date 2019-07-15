#include <fstream>

#include <glad/glad.h>

#include <ecvl/core.h>
#include <ecvl/gui.h>
#include <ecvl/gui/shader.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <wx/glcanvas.h>

#include "config.h"

using namespace ecvl;

template <DataType DT>
struct NormalizeToUint8Str {

    static void _(const Image& src, Image& dst) {

        dst.Create(src.dims_, DataType::uint8, src.channels_, src.colortype_, src.spacings_);

        ConstView<DT> src_v(src);
        View<DataType::uint8> dst_v(dst);

        // find max and min
        TypeInfo_t<DT> max = *std::max_element(src_v.Begin(), src_v.End());
        TypeInfo_t<DT> min = *std::min_element(src_v.Begin(), src_v.End());

        auto dst_it = dst_v.Begin();
        auto src_it = src_v.Begin();
        auto src_end = src_v.End();
        for (; src_it != src_end; ++src_it, ++dst_it) {
            (*dst_it) = (((*src_it) - min) * 255) / (max - min);
        }

    }

};

void NormalizeToUint8(const Image& src, Image& dst) {

    Table1D<NormalizeToUint8Str> table;
    table(src.elemtype_)(src, dst);

}

class BasicGLPane : public wxGLCanvas
{
    wxGLContext* m_context;
    wxTimer timer;
    ecvl::Shader ourShader, textShader;
    unsigned int VBO3D, VAO3D, EBO, texture3D, texture2D;
    const float radius = 0.7f;
    clock_t t;
    glm::mat4 view;
    glm::mat4 orientation;
    glm::mat4 ruota;
    glm::mat4 scala;
    bool enable_rotation;
    float fps = 30;
    float period = 5;
    float n = 10;
    float distance = 0.5f + 1/n;
    float glyph_dim = 1.f / 8;
    int xy_value = 0;
    int yz_value = 0;
    int xz_value = 0;
    int height;
    int width;
    int depth;
    float scale_w;
    float scale_h;
    float scale_d;

    std::string vertex_shader =
        "#version 330 core\n"
        "layout(location = 0) in vec2 xyPos;\n"
        "layout(location = 1) in vec3 aTexCoord;\n"

        "uniform float zPos;\n"
        "uniform float zTex;\n"
        "uniform mat4 model;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"

        "out vec2 TexCoord;\n"

        "void main()\n"
        "{"
        "    gl_Position = projection * view * model * vec4(xyPos, zPos, 1.0);"
        "    TexCoord = aTexCoord.xy;"
        "}";

    std::string fragment_shader =
        "#version 330 core\n"
        "out vec4 FragColor;\n"

        "in vec2 TexCoord;\n"

        "uniform sampler3D ourTexture;\n"
        "uniform float zTex;\n"
        "uniform mat4 orientation;\n"
        "uniform mat4 ruota;\n"
        "uniform float radius;\n"
        "uniform mat4 scala;\n"
        "uniform bool useAlpha;\n"

        "void main()"
        "{"
        "    vec4 color = texture(ourTexture, (scala * orientation * ruota * vec4(vec3(TexCoord, zTex), 1)).xyz + vec3(0.5f, 0.5f, 0.5f));"
        "    if(useAlpha) {\n"
        "       FragColor = color;\n"
        "    } else {\n"
        "       FragColor = vec4(color.xyz, 1.f);\n"
        "    }\n"
        //"    FragColor = vec4(1.f, 0, 0, 1);"
        "}";

    std::string text_vertex_shader =
        "#version 330 core\n"
        "layout(location = 0) in vec2 xyPos;\n"

        "uniform float zPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 projection;\n"
        "uniform mat4 view;\n"

        "out vec2 TexCoord;\n"

        "void main()\n"
        "{"
        //"    gl_Position = projection * view * model * vec4(xyPos / 5, zPos, 1.0);"
        "    gl_Position = projection * view * model * vec4(xyPos / 10, 0.f, 1.0);"
        "    TexCoord = vec2(xyPos.x, -xyPos.y) + vec2(0.5f, 0.5f);"
        "}";

    std::string text_fragment_shader =
        "#version 330 core\n"
        "out vec4 FragColor;\n"

        "in vec2 TexCoord;\n"

        "uniform vec2 glyphsCoord;\n"
        "uniform sampler2D ourTexture;\n"

        "void main()"
        "{"
        "    FragColor = texture(ourTexture, TexCoord / 8.2 + glyphsCoord);"
        "}";

public:
    BasicGLPane(wxPanel* parent, int* args, const ecvl::Image& img);
    virtual ~BasicGLPane();

    void OnTimer(wxTimerEvent& event);

    void Render(wxPaintEvent& evt);

    void SetViewport();
    void SetXY(const int& xy);
    void SetYZ(const int& yz);
    void SetXZ(const int& xz);

    void KeyReleased(wxKeyEvent& evt);
    void MouseWheelMoved(wxMouseEvent& evt);

    // events
    //void mouseMoved(wxMouseEvent& event);
    //void mouseDown(wxMouseEvent& event);
    //void mouseWheelMoved(wxMouseEvent& event);
    //void mouseReleased(wxMouseEvent& event);
    //void rightClick(wxMouseEvent& event);
    //void mouseLeftWindow(wxMouseEvent& event);
    //void keyPressed(wxKeyEvent& event);
    //void keyReleased(wxKeyEvent& event);

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(BasicGLPane, wxGLCanvas)
//EVT_MOTION(BasicGLPane::mouseMoved)
//EVT_LEFT_DOWN(BasicGLPane::mouseDown)
//EVT_LEFT_UP(BasicGLPane::mouseReleased)
//EVT_RIGHT_DOWN(BasicGLPane::rightClick)
//EVT_LEAVE_WINDOW(BasicGLPane::mouseLeftWindow)
//EVT_KEY_DOWN(BasicGLPane::keyPressed)
EVT_KEY_UP(BasicGLPane::KeyReleased)
EVT_MOUSEWHEEL(BasicGLPane::MouseWheelMoved)
EVT_PAINT(BasicGLPane::Render)
EVT_TIMER(wxID_ANY, BasicGLPane::OnTimer)
END_EVENT_TABLE()

BasicGLPane::BasicGLPane(wxPanel* parent, int* args, const Image& src_img) :
    wxGLCanvas(parent, wxID_ANY, args, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE), timer(this, wxID_ANY)
{
    m_context = new wxGLContext(this);

    SetCurrent(*m_context);

    if (!gladLoadGL()) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }

    Image uint8_conversion;
    NormalizeToUint8(src_img, uint8_conversion);
    const Image& img = uint8_conversion;

    //std::cout << GLVersion.major << "." << GLVersion.minor << std::endl;

    timer.Start(1000 / fps);
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices3D[] = {
        // positions         // texture coords
         0.5f,  0.5f, /*0.0f,*/  +radius, -radius,  // top right
         0.5f, -0.5f, /*0.0f,*/  +radius, +radius,  // bottom right
        -0.5f, -0.5f, /*0.0f,*/  -radius, +radius,  // bottom left
        -0.5f,  0.5f, /*0.0f,*/  -radius, -radius   // top left 
    };
    unsigned int indices3D[] = {
        0, 1, 2,
        0, 2, 3,
    };

    glGenVertexArrays(1, &VAO3D);
    glGenBuffers(1, &VBO3D);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO3D);

    glBindBuffer(GL_ARRAY_BUFFER, VBO3D);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices3D), vertices3D, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices3D), indices3D, GL_STATIC_DRAW);

    width = img.dims_[0];
    height = img.dims_[1];
    depth = img.dims_[2];

    float dw = 1;
    float dh = 1;
    float dd = 1;

    if (img.spacings_.size() >= 3) {
        dw = img.spacings_[0];
        dh = img.spacings_[1];
        dd = img.spacings_[2];
    }

    scale_w = (1.f / width) / dw;
    scale_h = (1.f / height) / dh;
    scale_d = (1.f / depth) / dd;

    float scale_min = std::min(std::min(scale_w, scale_h), scale_d);
    float coeff = 1 / scale_min;
    scale_w *= coeff;
    scale_h *= coeff;
    scale_d *= coeff;

    scala = glm::scale(glm::mat4(1.f), glm::vec3(scale_w, scale_h, scale_d));

    // Going 3D
    glGenTextures(1, &texture3D);
    glBindTexture(GL_TEXTURE_3D, texture3D);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Convert the texture data to RGBA
    unsigned char black_threshold = 30;
    unsigned char alpha = 255;
    unsigned char* data = new unsigned char[img.dims_[0] * img.dims_[1] * img.dims_[2] * 4];


    // !!! Only works with DataType::uint8 !!!
    if (img.colortype_ == ColorType::RGB)
    {
        for (int i = 0; i < img.dims_[0] * img.dims_[1] * img.dims_[2]; i++) {
            memcpy(data + i * 4, img.data_ + i * 3, 3);
            if (data[i * 4 + 0] < black_threshold && data[i * 4 + 1] < black_threshold && data[i * 4 + 2] < black_threshold) {
                data[i * 4 + 3] = 0;
            }
            else {
                data[i * 4 + 3] = data[i * 4];
                //data[i * 4 + 3] = alpha;
            }
        }
    }
    else if (img.colortype_ == ColorType::GRAY)
    {
        for (int i = 0; i < img.dims_[0] * img.dims_[1] * img.dims_[2]; i++) {
            data[i * 4] = img.data_[i];
            data[i * 4 + 1] = img.data_[i];
            data[i * 4 + 2] = img.data_[i];
            data[i * 4 + 3] = img.data_[i];
            //data[i * 4 + 3] = alpha;
            if (data[i * 4 + 3] < black_threshold) {
                data[i * 4 + 3] = 0;
            }
        }
    }

    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, width, height, depth, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    delete[] data;

    glGenerateMipmap(GL_TEXTURE_3D);

    glGenTextures(1, &texture2D);
    glBindTexture(GL_TEXTURE_2D, texture2D);

    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Image texture;
    ImRead(kSourceDir / filesystem::path("ExportedFont.bmp"), texture);
    RearrangeChannels(texture, texture, "cxy");

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texture.dims_[1], texture.dims_[2], 0, GL_BGR, GL_UNSIGNED_BYTE, texture.data_);
    texture = Image();

    glGenerateMipmap(GL_TEXTURE_2D);

    // Transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // render container
    ourShader.init(vertex_shader, fragment_shader);
    ourShader.use();

    textShader.init(text_vertex_shader, text_fragment_shader);

    orientation = glm::mat4(1.f);
    ourShader.setMat4("orientation", orientation);

    ruota = glm::mat4(1.f);
    enable_rotation = false;

    view = glm::mat4(1.0f);
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, 0.f));
    //view = glm::rotate(view, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
    ourShader.setMat4("view", view);

    glm::mat4 projection;
    //projection = glm::perspective(glm::radians(35.f), (float)GetSize().x / (float)GetSize().y, 0.1f, 100.0f);  // projective
    projection = glm::ortho(-1.f - 2/n, 1.f + 2 / n, -1.f - 1 / n - 2/n, 1.f + 1 / n);                                                          // orthographic
    ourShader.setMat4("projection", projection);

    ourShader.setFloat("radius", radius);
    ourShader.setMat4("scala", scala);

    textShader.use();
    textShader.setMat4("projection", projection);
    textShader.setMat4("view", view);

    // To avoid flashing on MSW
    SetBackgroundStyle(wxBG_STYLE_CUSTOM);
}

BasicGLPane::~BasicGLPane()
{
    glDeleteVertexArrays(1, &VAO3D);
    glDeleteBuffers(1, &VBO3D);
    glDeleteBuffers(1, &EBO);

    delete m_context;
}

void BasicGLPane::Render(wxPaintEvent& evt)
{
    if (!IsShown()) return;

    wxPaintDC(this); // only to be used in paint events. use wxClientDC to paint outside the paint event

    SetViewport();

    // render
    // ------
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glBindVertexArray(VAO3D);

    glEnable(GL_TEXTURE_3D);
    glBindTexture(GL_TEXTURE_3D, texture3D);

    ourShader.use();

    if (enable_rotation) {
        ruota = glm::rotate(ruota, glm::radians(360.f / (period * fps)), glm::vec3(0.0f, 1.0f, 0.0f));
    }
    ourShader.setMat4("ruota", ruota);
    ourShader.setBool("useAlpha", true);
    ourShader.setMat4("scala", scala);
    ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(-distance, +distance, 0.f)));
    ourShader.setMat4("orientation", orientation);
    
    const int slices = 200;

    for (int i = 0; i < slices; i++) { 
        float z = -radius + (radius / slices) + i * ((radius * 2) / slices); // Z-coordinate of the quad slice (and of the texture slice)
    
        ourShader.setFloat("zPos", z);
        ourShader.setFloat("zTex", z);
    
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);   
    }


    // Sections
    
    ourShader.setBool("useAlpha", false);
    ourShader.setMat4("scala", glm::mat4(1.f));
    ourShader.setMat4("orientation", glm::mat4(1.f));

    // asse z
    ourShader.setMat4("scala", glm::scale(glm::mat4(1.f), glm::vec3(scale_w, scale_h, 1.f)));
    ourShader.setFloat("zTex", (1.f / depth) * xy_value - 0.5 + (1.f / (2 * depth)));
    ourShader.setFloat("zPos", 0);
    ourShader.setMat4("ruota", glm::mat4(1.0f));    
    ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+distance, +distance, 0.f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); 
    
    // asse x
    ourShader.setMat4("scala", glm::scale(glm::mat4(1.f), glm::vec3(1.f, scale_h, scale_d)));
    ourShader.setFloat("zTex", (1.f / width) * yz_value - 0.5 + (1.f / (2 * width)));
    ourShader.setFloat("zPos", 0);
    ourShader.setMat4("ruota", glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.0f, 1.0f, 0.0f)));
    ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(-distance, -distance, 0.f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
    // asse y
    ourShader.setMat4("scala", glm::scale(glm::mat4(1.f), glm::vec3(scale_w, 1.f, scale_d)));
    ourShader.setFloat("zTex", (1.f / height) * xz_value - 0.5 + (1.f / (2 * height)));
    ourShader.setFloat("zPos", 0);
    ourShader.setMat4("ruota", glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1.0f, 0.0f, 0.0f)));
    ourShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+distance, -distance, 0.f)));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.use();

    //view = glm::mat4(1.0f);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+0.5f + 1/(2*n), 1/(2*n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(0.f, glyph_dim*5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+0.5f + 1/n + 1 / (2 * n), 1 / (2 * n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(glyph_dim, glyph_dim * 5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(-(+0.5f + 1 / n + 1 / (2 * n)), -1 - 3 / (2 * n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(glyph_dim, glyph_dim * 5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(-(+0.5f + 1 / (2 * n)), -1 - 3 / (2 * n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(glyph_dim*2, glyph_dim * 5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+0.5f + 1 / (2 * n), -1 - 3 / (2 * n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(0.f, glyph_dim * 5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    textShader.setMat4("model", glm::translate(glm::mat4(1.0f), glm::vec3(+0.5f + 1 / n + 1 / (2 * n), -1 - 3 / (2 * n), 0.f)));
    textShader.setVec2("glyphsCoord", glm::vec2(glyph_dim*2, glyph_dim * 5));
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    SwapBuffers();
}

void BasicGLPane::OnTimer(wxTimerEvent& event)
{
    // do whatever you want to do every second here
    Refresh();
    Update();
}

void BasicGLPane::MouseWheelMoved(wxMouseEvent& evt) {

    int mouse_rotation = evt.GetWheelRotation();
    view = glm::translate(view, glm::vec3(0.0f, 0.0f, (float)mouse_rotation / 1000));
    textShader.setMat4("view", view);
    ourShader.setMat4("view", view);
}

void BasicGLPane::KeyReleased(wxKeyEvent& evt) {
    ourShader.use();
    int key_code = evt.GetKeyCode();
    if (key_code == 82 /* R */ || key_code == WXK_SPACE) {
        enable_rotation = !enable_rotation;
    }
    else if (key_code == 87 /* W */) {
        orientation = glm::rotate(orientation, glm::radians(90.f), glm::vec3(1.f, 0.f, 0.f));
        ourShader.setMat4("orientation", orientation);
    }
    else if (key_code == 83 /* S */) {
        orientation = glm::rotate(orientation, glm::radians(-90.f), glm::vec3(1.f, 0.f, 0.f));
        ourShader.setMat4("orientation", orientation);
    }
    else if (key_code == 65 /* A */) {
        orientation = glm::rotate(orientation, glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f));
        ourShader.setMat4("orientation", orientation);
    }
    else if (key_code == 68 /* D */) {
        orientation = glm::rotate(orientation, glm::radians(-90.f), glm::vec3(0.f, 0.f, 1.f));
        ourShader.setMat4("orientation", orientation);
    }
}

void BasicGLPane::SetViewport() {
    wxSize s = GetSize();
    int min = std::min(s.x, s.y);
    glViewport((s.x - min) / 2, (s.y - min) / 2, min, min);
}

void BasicGLPane::SetXY(const int& xy)
{
    xy_value = xy;
}

void BasicGLPane::SetYZ(const int& yz)
{
    yz_value = yz;
}

void BasicGLPane::SetXZ(const int& xz)
{
    xz_value = xz;
}

enum
{
    ID_Load,
    ID_xy,
    ID_yz,
    ID_xz,
};

class MyFrame : public wxFrame
{
public:
    MyFrame();
private:
    ecvl::Image frame_img_;
    BasicGLPane *glPane;
    wxPanel *slider_panel;
    wxMenuBar *menuBar;
    wxBoxSizer *load_sizer;
    wxBoxSizer *topSizer;
    wxPanel *container;
    wxSlider *xy;
    wxSlider *yz;
    wxSlider *xz;
    void OnLoad(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSlider(wxCommandEvent& event);
};

MyFrame::MyFrame() : wxFrame(NULL, wxID_ANY, "ECVL Viewer", wxDefaultPosition, wxSize(770, 600))
{
    load_sizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer = new wxBoxSizer(wxHORIZONTAL);
    container = new wxPanel(this);
    container->Show(false);
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Load, "Load", "Choose one image from your filesystem");
    menuFile->AppendSeparator();

    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);
    CreateStatusBar();

    //SetStatusText("Welcome to wxWidgets!");
    Bind(wxEVT_MENU, &MyFrame::OnLoad, this, ID_Load);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);
    Bind(wxEVT_SLIDER, &MyFrame::OnSlider, this, ID_xy);
    Bind(wxEVT_SLIDER, &MyFrame::OnSlider, this, ID_yz);
    Bind(wxEVT_SLIDER, &MyFrame::OnSlider, this, ID_xz);
}

void MyFrame::OnLoad(wxCommandEvent& event)
{
    wxFileDialog
        openFileDialog(this, ("Open image"), "", "",
            "All files(*.nii, *.hdr) | *.nii; *.hdr; | NIFTI files(*.nii, *.hdr) | *.nii; *.hdr;", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
    std::string filename = openFileDialog.GetPath().ToStdString();
    NiftiRead(filename, frame_img_);

    if (frame_img_.colortype_ != ColorType::GRAY)
        ECVL_ERROR_NOT_IMPLEMENTED

    int args[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_DEPTH_SIZE, 16, 0 };

    load_sizer->Clear(true);
    topSizer->Clear();

    slider_panel = new wxPanel(container);

    wxStaticBoxSizer *label_sizer_xy = new wxStaticBoxSizer(new wxStaticBox(slider_panel, wxID_ANY, wxT("XY")), wxHORIZONTAL);
    wxStaticBoxSizer *label_sizer_yz = new wxStaticBoxSizer(new wxStaticBox(slider_panel, wxID_ANY, wxT("YZ")), wxHORIZONTAL);
    wxStaticBoxSizer *label_sizer_xz = new wxStaticBoxSizer(new wxStaticBox(slider_panel, wxID_ANY, wxT("XZ")), wxHORIZONTAL);
    xy = new wxSlider(slider_panel, ID_xy, 0, 0, frame_img_.dims_[2] - 1, wxPoint(20,0), wxSize(150, 50), wxSL_LABELS);
    yz = new wxSlider(slider_panel, ID_yz, 0, 0, frame_img_.dims_[0] - 1, wxPoint(20, 50), wxSize(150, 50), wxSL_LABELS);
    xz = new wxSlider(slider_panel, ID_xz, 0, 0, frame_img_.dims_[1] - 1, wxPoint(20, 100), wxSize(150, 50), wxSL_LABELS);
    label_sizer_xy->Add(xy);
    label_sizer_yz->Add(yz);
    label_sizer_xz->Add(xz);

    wxString test_str = "WASD: change the volume orientation.\nR or SPACE: start/stop rotation around y axis.";
    wxStaticText* text = new wxStaticText(slider_panel, wxID_ANY, test_str, wxPoint(20, 150));

    wxBoxSizer *vertical = new wxBoxSizer(wxVERTICAL);
    vertical->Add(label_sizer_xy, 0, wxALL, 10);
    vertical->Add(label_sizer_yz, 0, wxALL, 10);
    vertical->Add(label_sizer_xz, 0, wxALL, 10);
    vertical->Add(text, 0, wxALL, 10);
    slider_panel->SetSizer(vertical);

    glPane = new BasicGLPane(container, args, frame_img_);

    load_sizer->Add(glPane, 1, wxEXPAND);
    load_sizer->Add(slider_panel);

    container->Show(true);
    container->SetSizer(load_sizer);
    topSizer->Add(container, 1, wxEXPAND);
    SetSizer(topSizer);
    Layout();
}

void MyFrame::OnSlider(wxCommandEvent& event)
{
    switch (event.GetId())
    {
    case ID_xy:
    {
        glPane->SetXY(xy->GetValue());
        break;
    }
    case ID_yz:
    {
        glPane->SetYZ(yz->GetValue());
        break;
    }
    case ID_xz:
    {
        glPane->SetXZ(xz->GetValue());
        break;
    }
    }
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    Destroy();
    event.Skip();
}

void MyFrame::OnAbout(wxCommandEvent& event)
{
    wxMessageBox("ECVL Viewer - 2019",
        "About", wxOK | wxICON_INFORMATION);
}

class MyApp : public wxApp
{
public:
    virtual bool OnInit();
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    MyFrame *frame = new MyFrame();

    frame->SetSizer(sizer);
    frame->SetAutoLayout(true);

    frame->Show();
    return true;
}