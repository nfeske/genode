/*
 * \brief  Child subsystem representation
 * \author Norman Feske
 * \date   2017-08-23
 */

/*
 * Copyright (C) 2017 Genode Labs GmbH
 *
 * This file is part of the Genode OS framework, which is distributed
 * under the terms of the GNU Affero General Public License version 3.
 */

#ifndef _CHILD_H_
#define _CHILD_H_

/* local includes */
#include <dependency.h>

namespace Subspace_manager { struct Child; }


struct Subspace_manager::Child
{
	typedef String<128> Name;

	Allocator &_alloc;

	Constructible<Macro> _macro { _alloc };

	Managed_list<Dependency> _deps;

	Name const _name;

	Macro::Name const _macro_name;

	Binding _primary_binding;

	Child(Allocator &alloc, Name name, Macro::Name macro, Binding primary)
	:
		_alloc(alloc), _deps(alloc), _name(name), _macro_name(macro),
		_primary_binding(primary)
	{ }

	bool macro_defined() const { return _macro.constructed(); }

	bool runtime_defined() const
	{
		return macro_defined() && _macro->runtime_defined();
	}

	Macro::Name macro_name() const { return _macro_name; }

	Runtime::Name runtime_name() const
	{
		return macro_defined() ? _macro->runtime_name() : Runtime::Name();
	}

	/*
	 * Define macro for the child
	 *
	 * The macro provides the information about the used runtime
	 * and the bindings for the runtime's resources
	 */
	void macro(Xml_node macro)
	{
		if (_macro.constructed())
			return;

		if (macro.attribute_value("name", Macro::Name()) != _macro_name)
			return;

		_macro.construct(_alloc, macro);
	}

	/*
	 * Define runtime for the child
	 */
	void runtime(Xml_node runtime)
	{
		if (!_macro.constructed() || _macro->runtime_defined() ||
		    (_macro->runtime_name() != runtime.attribute_value("name", Runtime::Name())))
			return;

		_macro->define_runtime(runtime);
	}

	bool _runtime_arg_connected(Runtime::Arg const &arg) const
	{
		bool result = false;
		_deps.for_each([&] (Dependency const &dep) {
			if (dep.matches(arg))
				result = true; });

		return result;
	}

	template <typename FN>
	void connect(Binding_list const &global, FN const &fn)
	{
		if (!runtime_defined())
			return;

		_macro->_runtime->_expected_resources.for_each([&] (Runtime::Arg arg) {

			/* skip already connected bindings */
			if (_runtime_arg_connected(arg))
				return;

			/* connect primary binding */
			if (_primary_binding.matches(arg)) {
				_deps.append(Dependency::PRIMARY, arg,
				             fn(_primary_binding._target));
				return;
			}

			auto add_dependency = [&] (Binding const &binding) {
				_deps.append(Dependency::SECONDARY, arg, fn(binding._target)); };

			/* connect bindings defined by the child's macro */
			if (_macro->resolve(arg, add_dependency))
				return;

			/* connect global bindings */
			if (global.resolve(arg, add_dependency))
				return;
		});

		log("list of dependencies:");
		_deps.for_each([&] (Dependency const &dep) {
			log(" dep: ", dep._primary == Dependency::PRIMARY ? "PRIMARY" : "SECONDARY",
			    " arg: label=", dep._arg.label(), " type=", dep._arg.type());
		});
	}
};

#endif /* _CHILD_H_ */

