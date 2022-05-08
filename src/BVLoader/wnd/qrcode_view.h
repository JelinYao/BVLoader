#pragma once

class QREncode;
class QrcodeUI;
class QrcodeView
    : public CHorizontalLayoutUI
    , public IDialogBuilderCallback
{
    enum QrcodePage {
        PAGE_LOADING = 0,
        PAGE_QRCODE,
        PAGE_EXPIRE,
    };
public:
    QrcodeView(CPaintManagerUI* paint_manager);
    ~QrcodeView();

    void ShowLoading();
    void SetQrcodeUrl(std_str_r_ref url);
    void ShowExpire();

protected:
    void Init() override;
    CControlUI* CreateControl(LPCTSTR pstrClass) override;

private:
    CTabLayoutUI* tab_ = nullptr;
    QrcodeUI* ctrl_qrcode_ = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
// qrcode control
class QrcodeUI
    : public CControlUI
{
public:
    QrcodeUI();
    ~QrcodeUI();

    void UpdateQrcode(std_str_r_ref text);

protected:
    void Init() override;
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl) override;

private:
    std_str text_;
    DWORD back_color_ = 0xFFFFFF;
    DWORD qrcode_color_ = 0x000000;
    std::unique_ptr<QREncode> qrencode_;
    HDC mem_dc_ = NULL;
    HBITMAP mem_bitmap_ = NULL;
    float x_rate_ = 0.0F;
    float y_rate_ = 0.0F;
};