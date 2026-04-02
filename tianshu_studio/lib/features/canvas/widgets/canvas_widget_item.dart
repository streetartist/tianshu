import 'package:flutter/material.dart';
import 'package:flutter_riverpod/flutter_riverpod.dart';
import '../../../data/models/models.dart';
import '../state/canvas_notifier.dart';
import 'widget_renderers.dart';

/// 画布上的控件项
class CanvasWidgetItem extends ConsumerStatefulWidget {
  final WidgetInstance widget;
  final bool isSelected;

  const CanvasWidgetItem({
    super.key,
    required this.widget,
    required this.isSelected,
  });

  @override
  ConsumerState<CanvasWidgetItem> createState() => _CanvasWidgetItemState();
}

class _CanvasWidgetItemState extends ConsumerState<CanvasWidgetItem> {
  @override
  Widget build(BuildContext context) {
    return Positioned(
      left: widget.widget.x,
      top: widget.widget.y,
      child: GestureDetector(
        onTap: () {
          ref.read(canvasProvider.notifier).selectWidget(widget.widget.id);
        },
        onPanUpdate: (details) {
          ref.read(canvasProvider.notifier).moveWidget(
                widget.widget.id,
                widget.widget.x + details.delta.dx,
                widget.widget.y + details.delta.dy,
              );
        },
        child: _buildWidgetContainer(),
      ),
    );
  }

  Widget _buildWidgetContainer() {
    return Container(
      width: widget.widget.width,
      height: widget.widget.height,
      decoration: BoxDecoration(
        border: Border.all(
          color: widget.isSelected ? Colors.blue : Colors.grey.shade300,
          width: widget.isSelected ? 2 : 1,
        ),
        borderRadius: BorderRadius.circular(4),
      ),
      child: WidgetRenderers.render(widget.widget),
    );
  }
}
