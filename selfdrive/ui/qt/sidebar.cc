#include "selfdrive/ui/qt/sidebar.h"

#include <QMouseEvent>

#include "selfdrive/ui/qt/util.h"

void Sidebar::drawMetric(QPainter &p, const QPair<QString, QString> &label, QColor c, int y) {
  const QRect rect = {30, y, 240, 126};

  p.setPen(Qt::NoPen);
  p.setBrush(QBrush(c));
  p.setClipRect(rect.x() + 4, rect.y(), 18, rect.height(), Qt::ClipOperation::ReplaceClip);
  p.drawRoundedRect(QRect(rect.x() + 4, rect.y() + 4, 100, 118), 18, 18);
  p.setClipping(false);

  QPen pen = QPen(QColor(0xff, 0xff, 0xff, 0x55));
  pen.setWidth(2);
  p.setPen(pen);
  p.setBrush(Qt::NoBrush);
  p.drawRoundedRect(rect, 20, 20);

  p.setPen(QColor(0xff, 0xff, 0xff));
  p.setFont(InterFont(35, QFont::DemiBold));
  p.drawText(rect.adjusted(22, 0, 0, 0), Qt::AlignCenter, label.first + "\n" + label.second);
}

Sidebar::Sidebar(QWidget *parent) : QFrame(parent), onroad(false), flag_pressed(false), settings_pressed(false) {
  home_img = loadPixmap("../assets/images/button_home.png", home_btn.size());
  flag_img = loadPixmap("../assets/images/button_flag.png", home_btn.size());
  settings_img = loadPixmap("../assets/images/button_settings.png", settings_btn.size(), Qt::IgnoreAspectRatio);

  connect(this, &Sidebar::valueChanged, [=] { update(); });

  setAttribute(Qt::WA_OpaquePaintEvent);
  setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  setFixedWidth(300);

  QObject::connect(uiState(), &UIState::uiUpdate, this, &Sidebar::updateState);

  pm = std::make_unique<PubMaster, const std::initializer_list<const char *>>({"userFlag"});
  
  // #custom
  m_pSideBar = CSidebar( this );
}

void Sidebar::mousePressEvent(QMouseEvent *event) {
  if (onroad && home_btn.contains(event->pos())) {
    flag_pressed = true;
    update();
  } else if (settings_btn.contains(event->pos())) {
    settings_pressed = true;
    update();
  }
}

void Sidebar::mouseReleaseEvent(QMouseEvent *event) {
  if (flag_pressed || settings_pressed) {
    flag_pressed = settings_pressed = false;
    update();
  }
  if (home_btn.contains(event->pos())) {
    MessageBuilder msg;
    msg.initEvent().initUserFlag();
    pm->send("userFlag", msg);
  } else if (settings_btn.contains(event->pos())) {
    emit openSettings();
  }
}

void Sidebar::offroadTransition(bool offroad) {
  onroad = !offroad;
  update();
}

void Sidebar::updateState(const UIState &s) {
  if (!isVisible()) return;

  auto &sm = *(s.sm);

  auto deviceState = sm["deviceState"].getDeviceState();
  setProperty("netType", network_type[deviceState.getNetworkType()]);
  int strength = (int)deviceState.getNetworkStrength();
  setProperty("netStrength", strength > 0 ? strength + 1 : 0);

  ItemStatus connectStatus;
  auto last_ping = deviceState.getLastAthenaPingTime();
  if (last_ping == 0) {
    connectStatus = ItemStatus{{tr("CONNECT"), tr("OFFLINE")}, warning_color};
  } else {
    connectStatus = nanos_since_boot() - last_ping < 80e9
                        ? ItemStatus{{tr("CONNECT"), tr("ONLINE")}, good_color}
                        : ItemStatus{{tr("CONNECT"), tr("ERROR")}, danger_color};
  }
  setProperty("connectStatus", QVariant::fromValue(connectStatus));

  ItemStatus tempStatus = {{tr("TEMP"), tr("HIGH")}, danger_color};
  auto ts = deviceState.getThermalStatus();
  if (ts == cereal::DeviceState::ThermalStatus::GREEN) {
    tempStatus = {{tr("TEMP"), tr("GOOD")}, good_color};
  } else if (ts == cereal::DeviceState::ThermalStatus::YELLOW) {
    tempStatus = {{tr("TEMP"), tr("OK")}, warning_color};
  }
  setProperty("tempStatus", QVariant::fromValue(tempStatus));

  ItemStatus pandaStatus = {{tr("VEHICLE"), tr("ONLINE")}, good_color};
  if (s.scene.pandaType == cereal::PandaState::PandaType::UNKNOWN) {
    pandaStatus = {{tr("NO"), tr("PANDA")}, danger_color};
  } else if (s.scene.started && !sm["liveLocationKalman"].getLiveLocationKalman().getGpsOK()) {
    pandaStatus = {{tr("GPS"), tr("SEARCH")}, warning_color};
  }
  setProperty("pandaStatus", QVariant::fromValue(pandaStatus));

  // #custom
  if( m_pSideBar )
     m_pSideBar->updateState(s);   
}

void Sidebar::paintEvent(QPaintEvent *event) {
  QPainter p(this);
  p.setPen(Qt::NoPen);
  p.setRenderHint(QPainter::Antialiasing);

  p.fillRect(rect(), QColor(57, 57, 57));

  // buttons
  p.setOpacity(settings_pressed ? 0.65 : 1.0);
  p.drawPixmap(settings_btn.x(), settings_btn.y(), settings_img);
  p.setOpacity(onroad && flag_pressed ? 0.65 : 1.0);
  p.drawPixmap(home_btn.x(), home_btn.y(), onroad ? flag_img : home_img);
  p.setOpacity(1.0);

  // network
  int x = 58;
  const QColor gray(0x54, 0x54, 0x54);
  for (int i = 0; i < 5; ++i) {
    p.setBrush(i < net_strength ? Qt::white : gray);
    p.drawEllipse(x, 196, 27, 27);
    x += 37;
  }

  p.setFont(InterFont(35));
  p.setPen(QColor(0xff, 0xff, 0xff));
  const QRect r = QRect(50, 247, 100, 50);
  p.drawText(r, Qt::AlignCenter, net_type);

  // metrics
  drawMetric(p, temp_status.first, temp_status.second, 338);
  drawMetric(p, panda_status.first, panda_status.second, 496);
  drawMetric(p, connect_status.first, connect_status.second, 654);

  // #custom
  if( m_pSideBar )
    m_pSideBar->paintEvent( event );
}
