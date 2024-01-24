/*
 * \brief  Storage-device management widget
 * \author Norman Feske
 * \date   2018-04-30
 */

/*
 * Copyright (C) 2018-2023 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

/* local includes */
#include <view/storage_device_widget.h>

using namespace Sculpt;


namespace Dialog { struct Partition_button; }

struct Dialog::Partition_button : Widget<Hbox>
{
	void view(Scope<Hbox> &s, bool selected, Storage_target const &used_target,
	          Storage_device const &device, Partition const &partition) const
	{
		bool const hovered = s.hovered();

		auto color = [&] (bool selected, bool hovered)
		{
			if (selected) return Color { 255, 255, 255 };
			if (hovered)  return Color { 255, 255, 200 };
			else          return Color { 150, 150, 150 };
		};

		s.sub_scope<Left_floating_hbox>([&] (Scope<Hbox, Left_floating_hbox> &s) {

			s.sub_scope<Button>([&] (Scope<Hbox, Left_floating_hbox, Button> &s) {
				s.attribute("style", "invisible");
				s.sub_scope<Label>(partition.number, [&] (auto &s) {
					s.attribute("font", "text/metal");
					s.attribute("color", String<30>(color(selected, hovered)));
				});
			});

			if (partition.label.length() > 1)
				s.sub_scope<Label>(String<80>(" (", partition.label, ") "), [&] (auto &s) {
					s.attribute("font", "text/metal");
					s.attribute("color", String<30>(color(selected, hovered)));
				});

			Storage_target const target { device.label, partition.number };
			if (used_target == target)
				s.sub_scope<Label>("* ", [&] (auto &s) {
					s.attribute("font", "text/metal");
					s.attribute("color", String<30>(color(selected, hovered)));
				});
		});

		s.sub_scope<Right_floating_hbox>([&] (Scope<Hbox, Right_floating_hbox> &s) {
			s.sub_scope<Label>(String<64>(partition.capacity, " "), [&] (auto &s) {
				s.attribute("font", "text/metal");
				s.attribute("color", String<30>(color(selected, hovered)));
			}); });
	}
};


void Storage_device_widget::view(Scope<Vbox> &s, Storage_device const &dev,
                                 Storage_target const &used_target) const
{
	dev.partitions.for_each([&] (Partition const &partition) {

		bool const selected = (partition.number == _selected_partition);

		Hosted<Vbox, Partition_button> button { Id { partition.number } };
		s.widget(button, selected, used_target, dev, partition);

		if (selected)
			_partition_operations.view(s, dev, partition, used_target);
	});

	if (!_selected_partition.valid()) {
		s.sub_scope<Button_vgap>();
		_partition_operations.view(s, dev, *dev.whole_device_partition, used_target);
	}
}
