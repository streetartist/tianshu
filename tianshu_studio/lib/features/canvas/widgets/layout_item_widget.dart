import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/layout_models.dart';
import '../../widgets/widget_type.dart';
import '../state/layout_canvas_notifier.dart';
import 'canvas_widget_renderer.dart';

/// 布局项控件
class LayoutItemWidget extends ConsumerWidget {
  final LayoutItem item;
  final bool isSelected;

  const LayoutItemWidget({
    super.key,
    required this.item,
    required this.isSelected,
  });

  @override
  Widget build(BuildContext context, WidgetRef ref) {
    if (item.type == 'container') {
      return _buildContainerItem(context, ref);
    }
    return _buildDraggableItem(context, ref);
  }

  /// 可拖动的普通控件
  Widget _buildDraggableItem(BuildContext context, WidgetRef ref) {
    return Draggable<LayoutItem>(
      data: item,
      feedback: Material(
        elevation: 8,
        borderRadius: BorderRadius.circular(8),
        child: Opacity(
          opacity: 0.8,
          child: SizedBox(
            width: 200,
            child: CanvasWidgetRenderer.render(item),
          ),
        ),
      ),
      childWhenDragging: Opacity(
        opacity: 0.3,
        child: _buildItemContent(ref),
      ),
      child: _buildItemContent(ref),
    );
  }

  Widget _buildItemContent(WidgetRef ref) {
    return GestureDetector(
      onTap: () => ref.read(layoutCanvasProvider.notifier).selectItem(item.id),
      child: Container(
        margin: const EdgeInsets.only(bottom: 8),
        decoration: _buildDecoration(),
        child: CanvasWidgetRenderer.render(item),
      ),
    );
  }

  /// 容器控件 - 支持拖入子控件
  Widget _buildContainerItem(BuildContext context, WidgetRef ref) {
    return GestureDetector(
      onTap: () => ref.read(layoutCanvasProvider.notifier).selectItem(item.id),
      child: DragTarget<Object>(
        onWillAcceptWithDetails: (details) {
          // 接受 WidgetType（面板拖入）或 LayoutItem（页面拖入）
          return details.data is WidgetType || details.data is LayoutItem;
        },
        onAcceptWithDetails: (details) {
          final notifier = ref.read(layoutCanvasProvider.notifier);
          if (details.data is WidgetType) {
            // 从面板拖入新控件
            notifier.addItemToContainer(item.id, (details.data as WidgetType).name);
          } else if (details.data is LayoutItem) {
            // 从页面移动已有控件到容器
            notifier.moveItemToContainer(item.id, (details.data as LayoutItem).id);
          }
        },
        builder: (context, candidateData, rejectedData) {
          final isHovering = candidateData.isNotEmpty;
          return Container(
            margin: const EdgeInsets.only(bottom: 8),
            decoration: _buildDecoration(highlight: isHovering),
            child: Stack(
              children: [
                CanvasWidgetRenderer.renderContainer(item, ref),
                if (isHovering) _buildDropHint(),
              ],
            ),
          );
        },
      ),
    );
  }

  BoxDecoration _buildDecoration({bool highlight = false}) {
    return BoxDecoration(
      border: Border.all(
        color: highlight ? Colors.green : (isSelected ? Colors.blue : Colors.transparent),
        width: 2,
      ),
      borderRadius: BorderRadius.circular(8),
    );
  }

  Widget _buildDropHint() {
    return Positioned(
      bottom: 8,
      left: 0,
      right: 0,
      child: Center(
        child: Container(
          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
          decoration: BoxDecoration(
            color: Colors.green,
            borderRadius: BorderRadius.circular(4),
          ),
          child: const Text(
            '松开添加到容器',
            style: TextStyle(color: Colors.white, fontSize: 11),
          ),
        ),
      ),
    );
  }
}
