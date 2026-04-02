/// 控件类型枚举
enum WidgetType {
  // 显示控件
  numberDisplay('数值显示', 'display'),
  textDisplay('文本显示', 'display'),
  statusLight('状态指示灯', 'display'),
  progressBar('进度条', 'display'),
  gauge('仪表盘', 'display'),

  // 输入控件
  numberInput('数值输入', 'input'),
  slider('滑块', 'input'),
  dropdown('下拉选择', 'input'),

  // 控制控件
  switchButton('开关', 'control'),
  button('按钮', 'control'),

  // 图表控件
  lineChart('折线图', 'chart'),
  barChart('柱状图', 'chart'),

  // 布局控件
  container('容器', 'layout');

  final String label;
  final String category;

  const WidgetType(this.label, this.category);
}
