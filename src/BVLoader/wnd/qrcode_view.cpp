#include "pch.h"
#include "qrcode_view.h"
#include <utils/qrencode.h>

QrcodeView::QrcodeView(CPaintManagerUI* paint_manager)
{
    CDialogBuilder builder;
    auto root = dynamic_cast<CContainerUI*>(builder.Create(L"qrcode_view.xml", 0, this, paint_manager));
    if (root != nullptr) {
        Add(root);
    }
}

QrcodeView::~QrcodeView()
{
}

void QrcodeView::ShowLoading()
{
    tab_->SelectItem(PAGE_LOADING);
}

void QrcodeView::SetQrcodeUrl(std_str_r_ref url)
{
    assert(ctrl_qrcode_);
    ctrl_qrcode_->UpdateQrcode(std::move(url));
    tab_->SelectItem(PAGE_QRCODE);
}

void QrcodeView::ShowExpire()
{
    tab_->SelectItem(PAGE_EXPIRE);
}

void QrcodeView::Init()
{
    __super::Init();
    tab_ = dynamic_cast<CTabLayoutUI*>(FindSubControl(L"tab_qrcode"));
    assert(tab_);
}

CControlUI* QrcodeView::CreateControl(LPCTSTR pstrClass)
{
    if (wcscmp(pstrClass, L"QrcodeUI") == 0) {
        ctrl_qrcode_ = new QrcodeUI();
        return ctrl_qrcode_;
    }
    return nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
// qrcode control
QrcodeUI::QrcodeUI()
{
}

QrcodeUI::~QrcodeUI()
{
    if (mem_dc_) {
        ::DeleteDC(mem_dc_);
    }
    if (mem_bitmap_) {
        ::DeleteObject(mem_bitmap_);
    }
}

void QrcodeUI::UpdateQrcode(std_str_r_ref text)
{
    text_ = text;
    qrencode_->EncodeData(QR_LEVEL_H, 11, 1, 7, text_.c_str(), text_.size());
    NeedUpdate();
}

void QrcodeUI::Init()
{
    __super::Init();
    qrencode_ = std::make_unique<QREncode>();
}

bool QrcodeUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    if (text_.empty()) {
        return true;
    }
    int qrcode_width = qrencode_->m_nSymbleSize + (QR_MARGIN * 2);
    if (mem_dc_ == NULL) {
        mem_dc_ = ::CreateCompatibleDC(hDC);
        // BITMAPINFOHEADER bmiHeader = { sizeof(BITMAPINFOHEADER), qrcode_width, qrcode_width, 1, 32, BI_RGB };
        // mem_bitmap_ = CreateDIBSection(NULL, (BITMAPINFO*)&bmiHeader, DIB_RGB_COLORS, NULL, NULL, 0);3485FB
        mem_bitmap_ = ::CreateCompatibleBitmap(hDC, qrcode_width, qrcode_width);
        x_rate_ = qrcode_width / (float)GetWidth();
        y_rate_ = qrcode_width / (float)GetHeight();
    }
    HBITMAP old_bitmap = (HBITMAP)::SelectObject(mem_dc_, mem_bitmap_);
    // Ìî³ä±³¾°É«
    ::SetBkColor(mem_dc_, back_color_);
    RECT rc = { 0, 0, qrcode_width, qrcode_width };
    ::ExtTextOut(mem_dc_, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
    // »æÖÆ¶þÎ¬Âë
    for (int i = 0; i < qrencode_->m_nSymbleSize; ++i) {
        for (int j = 0; j < qrencode_->m_nSymbleSize; ++j) {
            if (qrencode_->m_byModuleData[i][j]) {
                ::SetPixel(mem_dc_, i + QR_MARGIN, j + QR_MARGIN, qrcode_color_);
            }
        }
    }
    //  À­ÉìÌùÍ¼
    int dest_width = rcPaint.right - rcPaint.left;
    int dest_height = rcPaint.bottom - rcPaint.top;
    int x = int((rcPaint.left - m_rcItem.left) * x_rate_);
    int y = int((rcPaint.top - m_rcItem.top) * y_rate_);

    ::StretchBlt(hDC, rcPaint.left, rcPaint.top, dest_width, dest_height,
        mem_dc_, x, y, int(dest_width * x_rate_), int(dest_height * y_rate_), SRCCOPY);

    ::SelectObject(mem_dc_, old_bitmap);
    return true;
}
