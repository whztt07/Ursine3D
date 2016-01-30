/* ---------------------------------------------------------------------------
** Team Bear King
** ?2015 DigiPen Institute of Technology, All Rights Reserved.
**
** AnimatorComponent.h
**
** Author:
** - Matt Yan - m.yan@digipen.edu
**
** Contributors:
** - <list in same format as author if applicable>
** -------------------------------------------------------------------------*/

#pragma once

#include "Component.h"
#include "Renderable.h"
#include "GfxAPI.h"
#include "AnimationBuilder.h"

namespace ursine
{
    namespace ecs
    {
        // not implemented yet
        class AnimationClip
        {
        public:
            enum MaskBlendMode
            {
                MASKBLEND_OVERRIDE = 0, // only override for now, can't blend multiple masks

                MASKBLEND_COUNT
            };

        public:
            const std::string &GetName(void) const;
            void SetName(const std::string &name);
            
            float GetStartTime(void) const;
            void SetStartTime(const float startTime);

            float GetEndTime(void) const;
            void SetEndTime(const float endTime);

            bool IsLooping(void);
            bool SetLooping(const bool looping);

            MaskBlendMode GetMaskBlendingMode(void) const;
            void SetMaskBlendingMode(const MaskBlendMode mode);

            ursine::AnimationState &GetAnimationState(void);
            void SetAnimationState(const ursine::AnimationState &state);

        private:
            // name of this clip
            std::string m_name; 

            // start time in the animation
            float m_start;

            // end time in the animation
            float m_end;

            // should we loop playing?
            bool m_looping;

            // how should we blend when applying multiple masks?
            MaskBlendMode m_maskBlendMode;

            // the animation state for this clip
            ursine::AnimationState m_animationState;
        };

        /////////////////////////////////////////////////////////////

        class Animator : public Component
        {
            NATIVE_COMPONENT;

        public:
			EditorButton( 
				AddState,
				"Add State"
				);

			EditorButton(
				RemoveState,
				"Remove State"
				);

			// let's don't care about in-state-blending. 
			// just care about between-state blending first.
			// let's try change animation state when timedelta reaches
			// at the end of that keyframe of the state

			// temporary
			// this is for testing just changing between states
			// not for changing between animations in state
			EditorButton(
				ChangeState,
				"Change State"
			);

			EditorButton(
				AddAnimation,
				"Add Animation"
			);

			EditorButton(
				RemoveAnimation,
				"Remove Animation"
			);

			EditorField(
				std::string stateName,
				GetStateName,
				SetStateName
			);

			EditorField(
				std::string currentState,
				GetCurrentState,
				SetCurrentState
			);		

			EditorField(
				std::string futureState,
				GetFutureState,
				SetFutureState
			);

            EditorField(
                std::string animationName,
                GetAnimation,
                SetAnimation
            );

            EditorField(
                std::string currentRig,
                GetRig,
                SetRig
            );

            EditorField(
                bool loopAnimation,
                IsLooping,
                SetLooping
            );

            EditorField(
                bool playAnimation,
                IsPlaying,
                SetPlaying
            );

            EditorField(
                bool renderDebug,
                IsDebug,
                SetDebug
            );

            EditorField(
                float timeScalar,
                GetTimeScalar,
                SetTimeScalar
            );

        public:
            Animator(void);
            ~Animator(void);

            Meta( Disable )
                void OnInitialize(void) override;

            // stick this in a system
            void UpdateAnimation(const float dt);

            // getter / setter //////////////////////////////////////
            bool IsPlaying(void) const;
            void SetPlaying(const bool isPlaying);

            bool IsLooping(void) const;
            void SetLooping(const bool isLooping);

            bool IsDebug(void) const;
            void SetDebug(const bool useDebug );

            float GetTimeScalar(void) const;
            void SetTimeScalar(const float scalar);

            const std::string &GetRig( void ) const;
            void SetRig(const std::string &rig);

            float GetAnimationTimePosition(void) const;
            void SetAnimationTimePosition(const float position);
			
			// Add/Remove State ("State Name (ex Run)", "Asset Name (ex Run@Player.FBX)")
			// start with adding/removing state
			const std::string &GetCurrentState(void) const;
			void SetCurrentState(const std::string &state);

			const std::string &GetStateName(void) const;
			void SetStateName(const std::string &state);

			const std::string &GetFutureState(void) const;
			void SetFutureState(const std::string& name);

			const std::string &GetAnimation(void) const;
			void SetAnimation(const std::string &name);

			// CrossFade ("State Name", transition time, ...)
			
        private:
            std::unordered_map<std::string, AnimationState> m_states;
			bool m_playing;
            bool m_looping;
            bool m_debug;
            float m_speedScalar;
            std::string m_currentAnimation;
            std::string m_currentRig;
			std::string m_currentState;
			std::string m_futureState;

			std::string m_stateName;
			std::string m_futureStateName;
			std::string m_animationName;

        } Meta( Enable, DisplayName( "Animator" ) );
    }
}