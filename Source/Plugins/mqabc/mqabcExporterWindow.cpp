#include "pch.h"
#include "mqabcExporterWindow.h"
#include "SceneGraph/ABC/sgabc.h"

namespace mqusd {

mqabcExporterWindow::mqabcExporterWindow(MQBasePlugin* plugin)
    : super(MQGetMainWindow())
{
    m_plugin = plugin;

    m_options.export_blendshapes = false;
    m_options.export_skeletons = false;
    m_options.separate_xform = true;

    setlocale(LC_ALL, "");
    SetTitle(L"Export Alembic");
    SetOutSpace(0.2);

    double outer_margin = 0.3;
    double inner_margin = 0.3;

    MQFrame* vf = CreateVerticalFrame(this);
    vf->SetOutSpace(outer_margin);
    vf->SetInSpace(inner_margin);

    {
        MQGroupBox* group = CreateGroupBox(vf, L"Scale");

        MQFrame* hf = CreateHorizontalFrame(group);
        CreateLabel(hf, L"Scale Factor");
        m_edit_scale = CreateEdit(hf);
        m_edit_scale->SetNumeric(MQEdit::NUMERIC_DOUBLE);
        m_edit_scale->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Freeze");

        m_check_mirror = CreateCheckBox(group, L"Freeze Mirror");
        m_check_mirror->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_lathe = CreateCheckBox(group, L"Freeze Lathe");
        m_check_lathe->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_subdiv = CreateCheckBox(group, L"Freeze Subdiv");
        m_check_subdiv->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);
    }
    {
        MQGroupBox* group = CreateGroupBox(vf, L"Convert Options");

        m_check_flip_v = CreateCheckBox(group, L"Flip V");
        m_check_flip_v->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_flip_x = CreateCheckBox(group, L"Flip X");
        m_check_flip_x->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_flip_yz = CreateCheckBox(group, L"Flip YZ");
        m_check_flip_yz->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_flip_faces = CreateCheckBox(group, L"Flip Faces");
        m_check_flip_faces->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        m_check_merge = CreateCheckBox(group, L"Merge Meshes");
        m_check_merge->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);

        MQFrame* hf = CreateHorizontalFrame(group);
        MQLabel* space = CreateLabel(hf, L" ");
        space->SetWidth(32);
        m_check_merge_only_visible = CreateCheckBox(hf, L"Only Visible");
        m_check_merge_only_visible->AddChangedEvent(this, &mqabcExporterWindow::OnSettingsUpdate);
    }

    {
        m_button_ok = CreateButton(vf, L"OK");
        m_button_ok->AddClickEvent(this, &mqabcExporterWindow::OnOKClicked);
    }

    this->AddShowEvent(this, &mqabcExporterWindow::OnShow);
    this->AddHideEvent(this, &mqabcExporterWindow::OnHide);
}

BOOL mqabcExporterWindow::OnShow(MQWidgetBase* sender, MQDocument doc)
{
    SyncSettings();
    return 0;
}

BOOL mqabcExporterWindow::OnHide(MQWidgetBase* sender, MQDocument doc)
{
    return 0;
}

BOOL mqabcExporterWindow::OnSettingsUpdate(MQWidgetBase* sender, MQDocument doc)
{
    auto& opt = m_options;
    {
        auto str = mu::ToMBS(m_edit_scale->GetText());
        auto value = std::atof(str.c_str());
        if (value != 0.0)
            opt.scale_factor = (float)value;
    }
    opt.freeze_mirror = m_check_mirror->GetChecked();
    opt.freeze_lathe = m_check_lathe->GetChecked();
    opt.freeze_subdiv = m_check_subdiv->GetChecked();

    opt.flip_v = m_check_flip_v->GetChecked();
    opt.flip_x = m_check_flip_x->GetChecked();
    opt.flip_yz = m_check_flip_yz->GetChecked();
    opt.flip_faces = m_check_flip_faces->GetChecked();

    opt.merge_meshes = m_check_merge->GetChecked();
    opt.merge_only_visible = m_check_merge_only_visible->GetChecked();

    return 0;
}

BOOL mqabcExporterWindow::OnOKClicked(MQWidgetBase* sender, MQDocument doc)
{
    if (DoExport(doc)) {
        SetVisible(false);
        return true;
    }
    return false;
}

void mqabcExporterWindow::SyncSettings()
{
    const size_t buf_len = 128;
    wchar_t buf[buf_len];
    auto& opt = m_options;

    swprintf(buf, buf_len, L"%.3f", opt.scale_factor);
    m_edit_scale->SetText(buf);

    m_check_mirror->SetChecked(opt.freeze_mirror);
    m_check_lathe->SetChecked(opt.freeze_lathe);
    m_check_subdiv->SetChecked(opt.freeze_subdiv);

    m_check_flip_v->SetChecked(opt.flip_v);
    m_check_flip_x->SetChecked(opt.flip_x);
    m_check_flip_yz->SetChecked(opt.flip_yz);
    m_check_flip_faces->SetChecked(opt.flip_faces);

    m_check_merge->SetChecked(opt.merge_meshes);
    m_check_merge_only_visible->SetChecked(opt.merge_only_visible);
}


void mqabcExporterWindow::SetOutputPath(const std::wstring& path)
{
    m_out_path = path;
}

bool mqabcExporterWindow::DoExport(MQDocument doc)
{
    auto scene = CreateABCOScene();
    if (!scene) {
        MQShowError("Failed to create Alembic scene.");
        return false;
    }

    if (!scene->create(mu::ToMBS(m_out_path).c_str())) {
        scene = {};

        MQShowError("Failed to create Alembic file.\nPossible reason: path contains multi-byte characters.");
        return false;
    }

    auto exporter = std::make_shared<DocumentExporter>(m_plugin, scene.get(), &m_options);
    exporter->initialize(doc);
    exporter->write(doc, true);

    return true;
}

} // namespace mqusd
