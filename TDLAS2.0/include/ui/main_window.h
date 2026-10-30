#pragma once
#include "ui/application_role.h"
#include "runtime/acquisition_runtime.h"
#include "storage/data_storage.h"
#include "daq/daq_backend.h"
#include "camera/camera_session.h"
#include <QMainWindow>
#include <QJsonObject>
#include <QList>
#include <QMap>

class QComboBox; class QDoubleSpinBox; class QSpinBox; class QPushButton;
class QLabel; class QTabWidget; class QLineSeries; class QChart; class QFormLayout; class QTableWidget;
class QThread;
namespace tdlas {
class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(const QString& dataRoot, QWidget* parent = nullptr,
                        ApplicationRole role = ApplicationRole::Administrator,
                        const QString& accountName = QStringLiteral("admin"),
                        std::shared_ptr<IDaqBackend> daqBackend = {});
    ~MainWindow() override;
    void setNightTheme(bool night);
    AcquisitionRuntime* runtime() { return &runtime_; }
protected:
    void closeEvent(QCloseEvent*) override;
private:
    QWidget* makeSidebar();
    QWidget* makeChart(const QString& title, const QString& xTitle, const QString& yTitle, const QString& objectName, bool raw = false);
    void createPages();
    void updateControls();
    void refreshSnapshot();
    void refreshResults();
    void updateCustomerHistoryRange();
    void renderCurves(QChart*, const std::vector<Curve>&, const QStringList& names = {});
    void startAcquisition();
    void refreshDaqDevices();
    void applySelectedDevice();
    const DaqDeviceDescriptor* selectedDaqDevice() const;
    void exportData();
    void refreshCameraDevices();
    void toggleCameraConnection();
    void updateCameraControls();
    void showCameraSettings();
    void renderCameraFrame(const QImage& image);
    void resetDisplay();
    bool hasSessionData() const;
    AcquisitionConfig config() const;
    AcquisitionRuntime runtime_;
    QString dataRoot_;
    ApplicationRole role_ = ApplicationRole::Administrator;
    QString accountName_;
    QJsonObject tokens_;
    QComboBox *device_ = nullptr, *output_ = nullptr, *waveform_ = nullptr, *input_ = nullptr;
    QComboBox* simulationScenario_ = nullptr;
    QDoubleSpinBox *frequency_ = nullptr, *amplitude_ = nullptr, *offset_ = nullptr, *sampleRate_ = nullptr, *maximum_ = nullptr, *minimum_ = nullptr;
    QSpinBox* displaySamples_ = nullptr;
    QPushButton *start_ = nullptr, *stop_ = nullptr, *reset_ = nullptr, *theme_ = nullptr, *refreshDevices_ = nullptr;
    QLabel *status_ = nullptr, *diagnostics_ = nullptr, *notice_ = nullptr, *parameterHint_ = nullptr, *clock_ = nullptr;
    QTabWidget* tabs_ = nullptr;
    QWidget* parameters_ = nullptr;
    QFormLayout* parameterForm_ = nullptr;
    QList<QChart*> charts_;
    QList<QLabel*> rawStates_;
    QLineSeries* rawSeries_ = nullptr;
    QMap<QString, QChart*> resultCharts_;
    QMap<QString, QLabel*> resultStates_;
    QTableWidget* peakTable_ = nullptr;
    QPushButton* exportButton_ = nullptr;
    QPushButton* customerExport_ = nullptr;
    QComboBox* customerHistoryRange_ = nullptr;
    QComboBox* cameraDevice_ = nullptr;
    QPushButton *cameraRefresh_ = nullptr, *cameraConnect_ = nullptr, *cameraCapture_ = nullptr, *cameraRecord_ = nullptr;
    QPushButton *cameraSaveLocation_ = nullptr, *cameraSettingsButton_ = nullptr;
    QWidget* cameraDevicePanel_ = nullptr;
    QLabel *cameraState_ = nullptr, *cameraDetails_ = nullptr, *cameraSaveState_ = nullptr, *cameraPreviewTime_ = nullptr;
    QList<QLabel*> cameraPreviews_;
    QList<CameraDeviceCapability> cameraDevices_;
    CameraSettings cameraSettings_;
    CameraSession cameraSession_;
    ResultSnapshot renderedResults_;
    std::shared_ptr<AsyncRawArchive> currentArchive_;
    QThread* exportWorker_ = nullptr;
    QThread* discoveryWorker_ = nullptr;
    std::shared_ptr<IDaqBackend> daqBackend_;
    QMap<QString, DaqDeviceDescriptor> daqDevices_;
    QString discoveryNotice_;
    QTimer displayTimer_, clockTimer_;
    std::uint64_t renderedSequence_ = 0;
    std::string renderedRunId_;
    bool hasRendered_ = false, night_ = false, closePending_ = false;
    bool confirmationPending_ = false;
    bool activeSessionIsSimulation_ = true;
};
}
