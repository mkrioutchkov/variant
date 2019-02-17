#pragma once
#include "limited_local_buffer.h"
#include "type_trait_extensions.h"

#include <iostream>
#include <type_traits>
#include <utility>

namespace mdk
{
	namespace detail
	{
		using variant_buffer_t = limited_local_buffer<>;

		struct variant_operation_holder_i
		{
			virtual const std::type_info& type_info() const noexcept = 0;

			virtual void destroy(variant_buffer_t& dst) const = 0;

			virtual void copy_construct(variant_buffer_t&, const variant_buffer_t&) const = 0;
			virtual void copy_assign(variant_buffer_t&, const variant_buffer_t&) const = 0;

			virtual void move_construct(variant_buffer_t&, variant_buffer_t&) const noexcept = 0;
			virtual void move_assign(variant_buffer_t&, variant_buffer_t&) const noexcept = 0;

			virtual std::ostream& out_stream(std::ostream& stream, const variant_buffer_t& src) const = 0;

			// 1. Checks if dst and src are the same type
			// 2. If yes, then stops here
			// 3. Calls destructor at dst
			// 4. If dst needs more space, then reallocates. 
			// 5. End result is dst has enough space but junk memory inside it
			bool repurpose(variant_buffer_t& dst, const variant_operation_holder_i& src_var) const
			{
				if (type_info() == src_var.type_info())
					return false;

				destroy(dst);
				src_var.allocate(dst);
				return true;
			}
		protected:
			virtual size_t size() const noexcept = 0;
		private:
			void allocate(variant_buffer_t& dst) const
			{
				dst.resize(size());
			}
		};

		template<typename T>
		struct variant_operation_holder : variant_operation_holder_i
		{
			static auto& instance()
			{
				const static variant_operation_holder operations;
				return operations;
			}

			const std::type_info& type_info() const noexcept override final
			{
				return typeid(T);
			}

			void destroy(variant_buffer_t& dst) const override final
			{
				lvalue(dst).~T();
			}

			void copy_construct(variant_buffer_t& dst, const variant_buffer_t& src) const override final
			{
				new (dst) T(lvalue(src));
			}

			void copy_assign(variant_buffer_t& dst, const variant_buffer_t& src) const override final
			{
				lvalue(dst) = lvalue(src);
			}

			void move_construct(variant_buffer_t& dst, variant_buffer_t& src) const noexcept override final
			{
				new (dst) T(rvalue(src));
			}

			void move_assign(variant_buffer_t& dst, variant_buffer_t& src) const noexcept override final
			{
				lvalue(dst) = rvalue(src);
			}

			std::ostream& out_stream(std::ostream& stream, const variant_buffer_t& src) const override final
			{
				if constexpr (test_streamable<decltype(stream), decltype(lvalue(src))>(bool{}))
					stream << lvalue(src);
				else
					stream << "variant of type " << type_info().name() << std::endl;
				return stream;
			}
		private:
			size_t size() const noexcept override final { return sizeof(T); }
			variant_operation_holder() = default;

			static T& lvalue(variant_buffer_t& p)
			{
				return static_cast<T&>(*reinterpret_cast<T*>(static_cast<char*>(p)));
			}

			static const T& lvalue(const variant_buffer_t& p)
			{
				return static_cast<const T&>(*reinterpret_cast<const T*>(static_cast<const char*>(p)));
			}

			static T&& rvalue(variant_buffer_t& p)
			{
				return std::move(*reinterpret_cast<T*>(static_cast<char*>(p)));
			}
		};
	}

	// A polymorphic variant class. Not like std::variant as the list of types is determined at run-time
	// Probably most similar to Qt's QVariant
	// See dependency_injector for an example of usefulness 
	struct variant
	{
		template<typename T, typename RAW_T = remove_cvref_t<T>, typename = std::enable_if_t<!std::is_same_v<RAW_T, variant> > >
		variant(T&& value)
			: m_type_info(&detail::variant_operation_holder<RAW_T>::instance())
			, m_buffer(sizeof(RAW_T))
		{
			new (m_buffer) RAW_T(std::forward<RAW_T>(value));
		}

		variant(variant&& other)
			: m_type_info(other.m_type_info)
		{
			m_type_info->move_construct(m_buffer, other.m_buffer);
		}

		variant(const variant& other)
			: m_type_info(other.m_type_info),
			m_buffer(other.m_buffer.size())
		{
			m_type_info->copy_construct(m_buffer, other.m_buffer);
		}

		~variant()
		{
			m_type_info->destroy(m_buffer);
		}

		variant& operator=(variant&& other)
		{
			// different types -> destroy old then move construct instead of move assign
			// otherwise just move assign
			if (m_type_info->repurpose(m_buffer, *other.m_type_info))
			{
				m_type_info = other.m_type_info;
				m_type_info->move_construct(m_buffer, other.m_buffer);
			}
			else
				m_type_info->move_assign(m_buffer, other.m_buffer);

			return *this;
		}

		variant& operator=(const variant& other)
		{
			return operator=(variant(other));
		}

		std::ostream& out_stream(std::ostream& stream) const
		{
			return m_type_info->out_stream(stream, m_buffer);
		}

		template<typename T>
		remove_cvref_t<T>& force_cast() noexcept
		{
			return *reinterpret_cast<remove_cvref_t<T>*>(m_buffer.ptr());
		}

		template<typename T>
		const remove_cvref_t<T>& force_cast() const noexcept
		{
			return *reinterpret_cast<const remove_cvref_t<T>*>(m_buffer.ptr());
		}

		template<typename T>
		T* cast() noexcept
		{
			return value<T>(*this);
		}

		template<typename T>
		const T* cast() const noexcept
		{
			return value<const T>(*this);
		}
	private:
		template<typename T, typename VARIANT>
		static T* value(VARIANT& this_variant) noexcept
		{
			using RAW_T = remove_cvref_t<std::remove_pointer_t<T>>;
			if (typeid(RAW_T) == this_variant.m_type_info->type_info())
				return reinterpret_cast<T*>(this_variant.m_buffer.ptr());

			return nullptr;
		}

		const detail::variant_operation_holder_i* m_type_info = nullptr;
		detail::variant_buffer_t m_buffer;
	};

	// This is needed to make const VARIANT& var an explicit parameter - without actually marking it explicit, otherwise test_streamable doesn't work properly on GCC
	template<typename VARIANT>
	inline std::enable_if_t<std::is_same_v<remove_cvref_t<VARIANT>, variant>, std::ostream&> operator<<(std::ostream& stream, const VARIANT& var)
	{
		return var.out_stream(stream);
	}
}