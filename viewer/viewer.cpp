#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#include <wx/translation.h>
#include <wx/gbsizer.h>
#include <wx/bookctrl.h>

#endif
#ifdef __WXMSW__
#include <wx/msw/msvcrt.h>      // redefines the new() operator 
#endif

#include "ecvl/gui.h"
#include "ecvl/core.h"

using namespace ecvl;
enum
{
    ID_Load,
    ID_ContrastBrightness,
    ID_Slider_BC,
    ID_Slider_Rotate,
    ID_Slider_Rotate_Full,
    ID_Slider_Threshold,
    ID_Rotate_Right,
    ID_Rotate_Left,
    ID_Rotate,
    ID_Rotate_Right_Full,
    ID_Rotate_Left_Full,
    ID_Rotate_Full,
    ID_Flip,
    ID_Mirror,
    ID_Threshold,
    ID_Negative,
    ID_Radio_Box,
    ID_Button_Ok,
    ID_Button_Cancel,
    ID_Notebook
};

class wxImagePanel : public wxPanel
{
    wxImage wxImage_;
    Image ecvlImage_;
    wxBitmap resized_;
    int w_, h_;
    void PaintEvent(wxPaintEvent & evt);
    void Render(wxDC& dc);

public:
    wxImagePanel(wxPanel* parent, wxWindowID id) : wxPanel(parent, id) {}
    wxImagePanel(wxFrame* parent) : wxPanel(parent) {}
    wxImagePanel(wxNotebook* parent, wxWindowID id) : wxPanel(parent, id) {}
    void SetImage(const wxImage& img);
    wxImage GetWXImage();
    Image GetECVLImage();
    void OnSize(wxSizeEvent& event);
    bool IsEmpty();

    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(wxImagePanel, wxPanel)
EVT_PAINT(wxImagePanel::PaintEvent)
EVT_SIZE(wxImagePanel::OnSize)
END_EVENT_TABLE()

void wxImagePanel::SetImage(const wxImage& img)
{
    wxImage_ = img.Copy();
    ecvlImage_ = ImgFromWx(wxImage_);
}

wxImage wxImagePanel::GetWXImage()
{
    return wxImage_;
}

Image wxImagePanel::GetECVLImage()
{
    return ecvlImage_;
}

bool wxImagePanel::IsEmpty()
{
    if (wxImage_.IsOk())
        return false;
    else return true;
}

void wxImagePanel::PaintEvent(wxPaintEvent & evt)
{
    wxPaintDC dc(this);
    Render(dc);
    wxTheApp->OnExit();
}

void wxImagePanel::Render(wxDC&  dc)
{
    int neww, newh;
    dc.GetSize(&neww, &newh);
    resized_ = wxBitmap(wxImage_.Scale(neww, newh));
    w_ = neww;
    h_ = newh;
    dc.DrawBitmap(resized_, 0, 0, false);
}

void wxImagePanel::OnSize(wxSizeEvent& event)
{
    Refresh();
    event.Skip();
}

class MyFrame : public wxFrame
{
public:
    MyFrame();
    Image frame_img_;
    wxImage imwxt;
    wxImagePanel *original_panel;
    wxImagePanel *modified_panel;
    wxImagePanel *tmp_panel;
    wxPanel *container;
    wxPanel *child_panel;
    wxPanel *base_panel;
    wxSlider *slide_b;
    wxSlider *slide_c;
    wxRadioBox *m_radio;
    wxBoxSizer *load_sizer;
    wxBoxSizer *topSizer;
    wxBoxSizer *image_sizer;
    wxStaticBoxSizer *csizer;
    wxNotebook *notebook;
    wxFrame *tmp_frame;
    wxMenuBar *menuBar;
    std::vector<wxImagePanel*> img_panels;
    std::vector<wxBoxSizer*> sizers;
    std::vector<wxPanel*> panels;
    std::vector<std::string> names;
    std::string filename;
    int current_id;
    bool frameids = false;
    bool new_tab = true;
    bool load = false;

private:
    void OnLoad(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnSlider(wxCommandEvent& event);
    void OnModifyInFrame(wxCommandEvent& event);
    void OnModify(wxCommandEvent& event);
    void CreateChild(const int& id);
};

class ChildFrame : public wxFrame
{
public:
    ChildFrame(MyFrame* parent, const std::string& title);
private:
    MyFrame *c_parent;
    void OnChildExit(wxCloseEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void OnRadioButtons(wxCommandEvent& event);
};

ChildFrame::ChildFrame(MyFrame* parent, const std::string& title) : wxFrame(parent, wxID_ANY, title)
{
    Connect(wxEVT_CLOSE_WINDOW, wxCloseEventHandler(ChildFrame::OnChildExit));
    c_parent = parent;
    SetPosition(c_parent->GetPosition());
    Bind(wxEVT_RADIOBOX, &ChildFrame::OnRadioButtons, this, ID_Radio_Box);
    Bind(wxEVT_BUTTON, &ChildFrame::OnOk, this, ID_Button_Ok);
    Bind(wxEVT_BUTTON, &ChildFrame::OnCancel, this, ID_Button_Cancel);
}

void ChildFrame::OnRadioButtons(wxCommandEvent &event)
{
    switch (c_parent->m_radio->GetSelection())
    {
    case 0:
        c_parent->new_tab = true;
        break;

    case 1:
        c_parent->new_tab = false;
        break;
    }
}

void ChildFrame::OnOk(wxCommandEvent& event)
{
    if (c_parent->new_tab)
    {
        wxImage img = c_parent->modified_panel->GetWXImage();
        c_parent->base_panel = new wxPanel(c_parent->notebook);
        wxBoxSizer *image_sizer = new wxBoxSizer(wxHORIZONTAL);
        c_parent->original_panel = new wxImagePanel(c_parent->base_panel, wxWindowID(c_parent->img_panels.size()));
        c_parent->original_panel->SetImage(img);
        c_parent->original_panel->SetSize(img.GetWidth(), img.GetHeight());

        image_sizer->Add(c_parent->original_panel, 1, wxSHAPED | wxALIGN_CENTER);
        c_parent->base_panel->SetSizer(image_sizer);

        c_parent->img_panels.push_back(c_parent->original_panel);
        c_parent->sizers.push_back(image_sizer);
        c_parent->panels.push_back(c_parent->base_panel);
        std::string filename = c_parent->names[c_parent->current_id] + " - Modified";
        c_parent->notebook->AddPage(c_parent->base_panel, filename, true);
        c_parent->names.push_back(filename);
        c_parent->frameids = false;
        this->Destroy();
    }
    else
    {
        wxImage img = c_parent->modified_panel->GetWXImage();
        c_parent->img_panels[c_parent->current_id]->SetImage(img);
        c_parent->img_panels[c_parent->current_id]->Refresh();
        c_parent->sizers[c_parent->current_id]->Clear();
        c_parent->sizers[c_parent->current_id]->Add(c_parent->img_panels[c_parent->current_id], 1, wxSHAPED | wxALIGN_CENTER);
        c_parent->panels[c_parent->current_id]->SetSizer(c_parent->sizers[c_parent->current_id]);
        c_parent->frameids = false;
        this->Destroy();
    }
}

void ChildFrame::OnCancel(wxCommandEvent& event)
{
    c_parent->frameids = false;
    Destroy();
}

void ChildFrame::OnChildExit(wxCloseEvent& event)
{
    c_parent->frameids = false;
    Destroy();
}

MyFrame::MyFrame() : wxFrame(NULL, wxID_ANY, "ECVL Viewer", wxDefaultPosition, wxSize(500, 500))
{
    load_sizer = new wxBoxSizer(wxHORIZONTAL);
    topSizer = new wxBoxSizer(wxHORIZONTAL);
    container = new wxPanel(this);
    container->Show(false);
    notebook = new wxNotebook(container, ID_Notebook);
    wxMenu *menuFile = new wxMenu;
    menuFile->Append(ID_Load, "Load", "Choose one image from your filesystem");
    menuFile->AppendSeparator();
    menuFile->Append(ID_ContrastBrightness, "Contrast and Brightness");
    wxMenu *rotate = new wxMenu;
    rotate->Append(ID_Rotate, wxT("Rotate..."));
    rotate->Append(ID_Rotate_Left, wxT("Rotate left"));
    rotate->Append(ID_Rotate_Right, wxT("Rotate right"));
    rotate->Append(ID_Rotate_Full, wxT("Rotate full image..."));
    rotate->Append(ID_Rotate_Left_Full, wxT("Rotate left full image"));
    rotate->Append(ID_Rotate_Right_Full, wxT("Rotate right full image"));

    menuFile->AppendSubMenu(rotate, wxT("Rotate"));
    menuFile->Append(ID_Flip, "Flip");
    menuFile->Append(ID_Mirror, "Mirror");
    menuFile->Append(ID_Threshold, "Threshold");
    menuFile->Append(ID_Negative, "Negative");

    menuFile->Append(wxID_EXIT);

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append(wxID_ABOUT);
    menuBar = new wxMenuBar;
    menuBar->Append(menuFile, "&File");
    menuBar->Append(menuHelp, "&Help");
    SetMenuBar(menuBar);
    CreateStatusBar();

    Bind(wxEVT_MENU, &MyFrame::OnLoad, this, ID_Load);
    Bind(wxEVT_MENU, &MyFrame::OnModify, this, ID_ContrastBrightness);
    Bind(wxEVT_MENU, &MyFrame::OnModify, this, ID_Rotate);
    Bind(wxEVT_MENU, &MyFrame::OnModify, this, ID_Threshold);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Rotate_Right);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Rotate_Left);
    Bind(wxEVT_MENU, &MyFrame::OnModify, this, ID_Rotate_Full);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Rotate_Right_Full);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Rotate_Left_Full);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Flip);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Mirror);
    Bind(wxEVT_MENU, &MyFrame::OnModifyInFrame, this, ID_Negative);
    Bind(wxEVT_MENU, &MyFrame::OnAbout, this, wxID_ABOUT);
    Bind(wxEVT_MENU, &MyFrame::OnQuit, this, wxID_EXIT);
    Bind(wxEVT_SCROLL_THUMBRELEASE, &MyFrame::OnSlider, this, ID_Slider_BC);
    Bind(wxEVT_SCROLL_THUMBRELEASE, &MyFrame::OnSlider, this, ID_Slider_Rotate);
    Bind(wxEVT_SCROLL_THUMBRELEASE, &MyFrame::OnSlider, this, ID_Slider_Rotate_Full);
    Bind(wxEVT_SCROLL_THUMBRELEASE, &MyFrame::OnSlider, this, ID_Slider_Threshold);
}

void MyFrame::OnLoad(wxCommandEvent& event)
{
    if (frameids)
    {
        tmp_frame->Destroy();
        frameids = false;
    }
    wxFileDialog
        openFileDialog(this, ("Open image"), "", "",
            "All files(*.png, *.jpg, *.jpeg) | *.png; *.jpg; *.jpeg; | PNG files(*.png) | *.png; | JPEG and JPG files(*.jpg, *.jpeg) | *.jpg; *.jpeg;", wxFD_OPEN | wxFD_FILE_MUST_EXIST);
    if (openFileDialog.ShowModal() == wxID_CANCEL)
        return;
    filename = openFileDialog.GetPath().ToStdString();
    ImRead(filename, frame_img_);

    filename = filename.substr(filename.find_last_of("/\\") + 1);

    //if (frame_img_.dims_[0] > 1000 || frame_img_.dims_[1] > 1000)
    //    ResizeScale(frame_img_, frame_img_, { 0.3,0.3 });

    base_panel = new wxPanel(notebook);
    image_sizer = new wxBoxSizer(wxHORIZONTAL);

    original_panel = new wxImagePanel(base_panel, wxWindowID(img_panels.size()));
    original_panel->SetImage(WxFromImg(frame_img_));
    original_panel->SetSize(frame_img_.dims_[1], frame_img_.dims_[2]);

    image_sizer->Add(original_panel, 1, wxSHAPED | wxALIGN_CENTER);
    base_panel->SetSizer(image_sizer);

    notebook->AddPage(base_panel, filename, true);
    names.push_back(filename);

    container->Show(true);

    if (img_panels.empty())
    {
        load_sizer->Add(notebook, 1, wxEXPAND);
        container->SetSizer(load_sizer);
        topSizer->Add(container, 1, wxEXPAND);
        SetSizer(topSizer);
    }

    img_panels.push_back(original_panel);
    sizers.push_back(image_sizer);
    panels.push_back(base_panel);
    Layout();
}

void MyFrame::OnModify(wxCommandEvent& event)
{
    if (frame_img_.IsEmpty())
    {
        event.Skip();
        return;
    }
    if (frameids)
        tmp_frame->Destroy();
    new_tab = true;
    current_id = notebook->GetSelection();
    wxBoxSizer *vertical_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer *first_row_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *second_row_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer *third_row_sizer = new wxBoxSizer(wxHORIZONTAL);

    CreateChild(event.GetId());
    tmp_panel = new wxImagePanel(child_panel, wxID_ANY);
    modified_panel = new wxImagePanel(child_panel, wxID_ANY);

    //Image tmp; 
    //tmp = img_panels[current_id]->GetECVLImage();
    //RearrangeChannels(tmp, tmp, "xyc");
    //ChangeColorSpace(tmp, tmp, ColorType::BGR);
    //ResizeScale(tmp, tmp, { 0.7, 0.7 });
    //tmp_panel->SetImage(WxFromImg(tmp));
    //tmp_panel->SetSize(tmp.dims_[1], tmp.dims_[2]);

    tmp_panel->SetImage(img_panels[current_id]->GetWXImage());
    tmp_panel->SetSize(tmp_panel->GetECVLImage().dims_[1], tmp_panel->GetECVLImage().dims_[2]);

    first_row_sizer->Add(tmp_panel, 3, wxSHAPED | wxALIGN_CENTER | wxALL, 10);
    modified_panel->SetImage(img_panels[current_id]->GetWXImage());
    modified_panel->SetSize(tmp_panel->GetSize());
    first_row_sizer->Add(modified_panel, 3, wxSHAPED | wxALIGN_CENTER | wxALL, 10);

    vertical_sizer->Add(first_row_sizer, 1, wxGROW);

    csizer->Add(slide_b);

    wxString choices[] = { "Add a new tab", "Overwrite the old image" };

    m_radio = new wxRadioBox(child_panel, ID_Radio_Box, wxT("Save Option"), wxPoint(10, 10), wxDefaultSize, WXSIZEOF(choices), choices, 1, wxRA_SPECIFY_COLS);

    second_row_sizer->Add(csizer);
    second_row_sizer->Add(m_radio); 

    vertical_sizer->Add(second_row_sizer, 0, wxLEFT | wxBOTTOM, 10);

    wxButton *ok = new wxButton(child_panel, ID_Button_Ok, "Ok");
    wxButton *cancel = new wxButton(child_panel, ID_Button_Cancel, "Cancel");
    third_row_sizer->Add(ok);
    third_row_sizer->Add(cancel);

    vertical_sizer->Add(third_row_sizer, 0, wxLEFT | wxBOTTOM, 10);

    child_panel->SetSizer(vertical_sizer);
    child_panel->Layout();

    tmp_frame->Show();
    int w = GetClientSize().GetWidth() + 100;
    int h = GetClientSize().GetHeight() / 2 + 30 + second_row_sizer->GetSize().GetHeight() + third_row_sizer->GetSize().GetHeight();
    tmp_frame->SetSize(w, h);

    frameids = true;
}

void MyFrame::CreateChild(const int& id)
{
    switch (id)
    {
    case ID_Rotate:
    {
        tmp_frame = new ChildFrame(this, "Rotate");
        child_panel = new wxPanel(tmp_frame);
        csizer = new wxStaticBoxSizer(new wxStaticBox(child_panel, wxID_ANY, wxT("Change angle")), wxHORIZONTAL);
        slide_b = new wxSlider(child_panel, ID_Slider_Rotate, 0, 0, 360, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        break;
    }
    case ID_Rotate_Full:
    {
        tmp_frame = new ChildFrame(this, "Rotate Full Image");
        child_panel = new wxPanel(tmp_frame);
        csizer = new wxStaticBoxSizer(new wxStaticBox(child_panel, wxID_ANY, wxT("Change angle")), wxHORIZONTAL);
        slide_b = new wxSlider(child_panel, ID_Slider_Rotate_Full, 0, 0, 360, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        break;
    }
    case ID_Threshold:
    {
        tmp_frame = new ChildFrame(this, "Threshold");
        child_panel = new wxPanel(tmp_frame);
        csizer = new wxStaticBoxSizer(new wxStaticBox(child_panel, wxID_ANY, wxT("Change threshold and max value")), wxHORIZONTAL);
        slide_b = new wxSlider(child_panel, ID_Slider_Threshold, 0, 0, 255, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        slide_c = new wxSlider(child_panel, ID_Slider_Threshold, 0, 0, 255, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        csizer->Add(slide_c);
        csizer->Add(50, 20, 1);
        break;
    }
    case ID_ContrastBrightness:
    {
        tmp_frame = new ChildFrame(this, "Contrast and Brightness");
        child_panel = new wxPanel(tmp_frame);
        csizer = new wxStaticBoxSizer(new wxStaticBox(child_panel, wxID_ANY, wxT("Change Brightness and Contrast")), wxHORIZONTAL);
        slide_b = new wxSlider(child_panel, ID_Slider_BC, 0, 0, 100, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        slide_c = new wxSlider(child_panel, ID_Slider_BC, 100, 100, 300, wxDefaultPosition, wxSize(150, 50), wxSL_LABELS);
        csizer->Add(slide_c);
        csizer->Add(50, 20, 1);
        break;
    }
    }
}

void MyFrame::OnSlider(wxCommandEvent& event)
{
    current_id = notebook->GetSelection();
    Image tmp = img_panels[current_id]->GetECVLImage();
    float pos_b = slide_b->GetValue();
    if (tmp.colortype_ != ColorType::BGR)
        ChangeColorSpace(tmp, tmp, ColorType::BGR);

    switch (slide_b->GetId())
    {
    case ID_Slider_BC:
    {
        float pos_c = slide_c->GetValue() / 100.0;
        Mul(tmp, pos_c, tmp);
        Add(tmp, pos_b, tmp);
        break;
    }
    case ID_Slider_Rotate:
    {
        if (tmp.channels_ != "xyc")
            RearrangeChannels(tmp, tmp, "xyc");
        Rotate2D(tmp, tmp, pos_b);
        break;
    }
    case ID_Slider_Rotate_Full:
    {
        if (tmp.channels_ != "xyc")
            RearrangeChannels(tmp, tmp, "xyc");
        RotateFullImage2D(tmp, tmp, pos_b);
        break;
    }
    case ID_Slider_Threshold:
    {
        float pos_c = slide_c->GetValue();
        if (tmp.channels_ != "xyc")
            RearrangeChannels(tmp, tmp, "xyc");
        Threshold(tmp, tmp, pos_c, pos_b);
        break;
    }
    }

    imwxt = WxFromImg(tmp);
    modified_panel->SetImage(imwxt);
    tmp_frame->SetSize(tmp_frame->GetSize().GetWidth() + 1, tmp_frame->GetSize().GetHeight() + 1);
    tmp_frame->SetSize(tmp_frame->GetSize().GetWidth() - 1, tmp_frame->GetSize().GetHeight() - 1);

    modified_panel->Update();
}

void MyFrame::OnModifyInFrame(wxCommandEvent& event)
{
    if (frame_img_.IsEmpty())
    {
        event.Skip();
        return;
    }
    else
    {
        current_id = notebook->GetSelection();
        Image tmp = img_panels[current_id]->GetECVLImage();
        if (tmp.colortype_ != ColorType::BGR)
            ChangeColorSpace(tmp, tmp, ColorType::BGR);
        if (tmp.channels_ != "xyc")
            RearrangeChannels(tmp, tmp, "xyc");

        switch (event.GetId())
        {
        case ID_Rotate_Right:
        {
            Rotate2D(tmp, tmp, 90);
            break;
        }
        case ID_Rotate_Left:
        {
            Rotate2D(tmp, tmp, 270);
            break;
        }
        case ID_Rotate_Right_Full:
        {
            RotateFullImage2D(tmp, tmp, 90);
            break;
        }
        case ID_Rotate_Left_Full:
        {
            RotateFullImage2D(tmp, tmp, 270);
            break;
        }
        case ID_Flip:
        {
            Flip2D(tmp, tmp);
            break;
        }
        case ID_Mirror:
        {
            Mirror2D(tmp, tmp);
            break;
        }
        case ID_Negative:
        {
            Image tmp2;
            if (tmp.elemtype_ != DataType::int8)
                CopyImage(tmp, tmp2, DataType::int8);
            tmp = tmp2;
            Neg(tmp);
            break;
        }
        }

        imwxt = WxFromImg(tmp);
        img_panels[current_id]->SetImage(imwxt);
        img_panels[current_id]->SetSize(imwxt.GetWidth(), imwxt.GetHeight());
        sizers[current_id]->Clear();
        sizers[current_id]->Add(img_panels[current_id], 1, wxSHAPED | wxALIGN_CENTER);
        panels[current_id]->SetSizer(sizers[current_id]);

        SetSize(GetSize().GetWidth() + 1, GetSize().GetHeight() + 1);
        SetSize(GetSize().GetWidth() - 1, GetSize().GetHeight() - 1);
        img_panels[current_id]->Update();
    }
}

void MyFrame::OnQuit(wxCommandEvent& event)
{
    Destroy();
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
    wxInitAllImageHandlers();
    MyFrame *frame = new MyFrame();
    frame->Show(true);
    return true;
}