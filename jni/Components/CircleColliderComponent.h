#pragma once
#include "BaseComponent.h"
#include "..\defines.h"

namespace star
{
	struct Context;

	class CircleColliderComponent final : public BaseComponent
	{
	public:
		// [COMMENT] remove default value
		CircleColliderComponent(float radius = 1.0f);
		~CircleColliderComponent(void);

		void Initialize();
		void Update(const Context& context);
		void Draw();

		void SetAsTrigger(bool isTrigger) {m_bIsTrigger = isTrigger;};
		bool IsTrigger() const {return m_bIsTrigger;};

	protected:
		void InitializeComponent();

		float m_Radius;
		bool m_bIsTrigger;

	private:
		// -------------------------
		// Disabling default copy constructor and default 
		// assignment operator.
		// -------------------------
		CircleColliderComponent(const CircleColliderComponent& t);
		CircleColliderComponent& operator=(const CircleColliderComponent& t);
	};
}